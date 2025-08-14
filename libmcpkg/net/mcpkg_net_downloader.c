/* SPDX-License-Identifier: MIT */
#include "net/mcpkg_net_downloader.h"

#include "mcpkg_export.h"

#include "net/mcpkg_net_client.h"
#include "net/mcpkg_net_util.h"

#include "threads/mcpkg_thread_pool.h"
#include "threads/mcpkg_thread_future.h"
#include "threads/mcpkg_thread_promise.h"
#include "threads/mcpkg_thread_util.h"

#include "fs/mcpkg_fs_file.h"
#include "fs/mcpkg_fs_error.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct McPkgNetDownloader {
    struct McPkgNetClient   *cli;           /* borrowed */
    struct McPkgThreadPool  *pool;          /* owned if owns_pool != 0 */
    int                     owns_pool;
    char                    *download_dir;  /* optional copy */
};

/* ---------------- internal helpers ---------------- */

static int map_fs_to_net(int fs_err)
{
    switch (fs_err) {
    case MCPKG_FS_OK:           return MCPKG_NET_NO_ERROR;
    case MCPKG_FS_ERR_OOM:      return MCPKG_NET_ERR_NOMEM;
    case MCPKG_FS_ERR_NOSPC:    return MCPKG_NET_ERR_IO;
    case MCPKG_FS_ERR_EXISTS:   return MCPKG_NET_ERR_OTHER;
    case MCPKG_FS_ERR_NOT_FOUND:return MCPKG_NET_ERR_OTHER;
    case MCPKG_FS_ERR_RANGE:    return MCPKG_NET_ERR_OTHER;
    default:                    return MCPKG_NET_ERR_IO;
    }
}

static char *cdup(const char *s)
{
    size_t n;
    char *d;

    if (!s) return NULL;
    n = strlen(s) + 1U;
    d = (char *)malloc(n);
    if (!d) return NULL;
    memcpy(d, s, n);
    return d;
}

static int path_is_abs(const char *p)
{
    if (!p || !p[0]) return 0;
#if defined(_WIN32)
    if (p[0] == '/' || p[0] == '\\') return 1;
    if (((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')) &&
        p[1] == ':')
        return 1;
    return 0;
#else
    return (p[0] == '/');
#endif
}

static char *join_paths(const char *base, const char *rel)
{
    char *out;
    size_t nb, nr, need;
    int add_sep = 0;

    if (!base) return cdup(rel);
    if (!rel)  return cdup(base);

    nb = strlen(base);
    nr = strlen(rel);

    if (nr == 0) return cdup(base);
    if (nb == 0) return cdup(rel);

#if defined(_WIN32)
    {
        char last = base[nb - 1];
        add_sep = !(last == '/' || last == '\\');
    }
#else
    add_sep = (base[nb - 1] != '/');
#endif

    need = nb + (add_sep ? 1 : 0) + nr + 1;
    out = (char *)malloc(need);
    if (!out) return NULL;

    memcpy(out, base, nb);
    if (add_sep) {
#if defined(_WIN32)
        out[nb] = '\\';
#else
        out[nb] = '/';
#endif
        memcpy(out + nb + 1, rel, nr + 1);
    } else {
        memcpy(out + nb, rel, nr + 1);
    }
    return out;
}

/* ---------------- lifecycle ---------------- */

int mcpkg_net_downloader_new(const struct McPkgNetDownloaderCfg *cfg,
                             struct McPkgNetDownloader **out)
{
    struct McPkgNetDownloader *dl;
    int rc;

    if (!cfg || !cfg->client || !out)
        return MCPKG_THREAD_E_INVAL;

    dl = (struct McPkgNetDownloader *)calloc(1, sizeof(*dl));
    if (!dl)
        return MCPKG_THREAD_E_NOMEM;

    dl->cli = cfg->client;

    if (cfg->pool) {
        dl->pool = cfg->pool;
        dl->owns_pool = 0;
    } else {
        struct McPkgThreadPoolCfg pcfg;
        unsigned int th = cfg->parallel ? cfg->parallel : 4U;
        unsigned int q  = cfg->queue    ? cfg->queue    : 64U;

        pcfg.threads    = th;
        pcfg.q_capacity = q;

        rc = mcpkg_thread_pool_new(&pcfg, &dl->pool);
        if (rc != MCPKG_THREAD_NO_ERROR) {
            free(dl);
            return rc;
        }
        dl->owns_pool = 1;
    }

    if (cfg->download_dir) {
        dl->download_dir = cdup(cfg->download_dir);
        if (!dl->download_dir) {
            if (dl->owns_pool) {
                mcpkg_thread_pool_shutdown(dl->pool);
                mcpkg_thread_pool_free(dl->pool);
            }
            free(dl);
            return MCPKG_THREAD_E_NOMEM;
        }
    }

    *out = dl;
    return MCPKG_THREAD_NO_ERROR;
}

void mcpkg_net_downloader_free(struct McPkgNetDownloader *dl)
{
    if (!dl) return;
    if (dl->owns_pool && dl->pool) {
        (void)mcpkg_thread_pool_shutdown(dl->pool);
        mcpkg_thread_pool_free(dl->pool);
    }
    free(dl->download_dir);
    free(dl);
}

/* ---------------- task + enqueue ---------------- */

struct DlTask {
    struct McPkgNetClient           *cli;       /* borrowed */
    char                            *path;      /* GET path */
    char                           **query;     /* NULL-terminated copy */
    char                            *outfile;   /* absolute or joined */
};

static void strv_free(char **v)
{
    size_t i;
    if (!v) return;
    for (i = 0; v[i] != NULL; i++)
        free(v[i]);
    free(v);
}

static char **strv_dup(const char *const *v)
{
    size_t i, cnt = 0;
    char **out;

    if (!v) {
        out = (char **)calloc(1, sizeof(char *));
        return out; /* single NULL */
    }
    while (v[cnt] != NULL)
        cnt++;

    out = (char **)calloc(cnt + 1U, sizeof(char *));
    if (!out)
        return NULL;

    for (i = 0; i < cnt; i++) {
        out[i] = cdup(v[i]);
        if (!out[i]) {
            while (i > 0) {
                i--;
                free(out[i]);
            }
            free(out);
            return NULL;
        }
    }
    /* out[cnt] already NULL */
    return out;
}

static int dl_task_run(void *arg, void **out_result, int *out_err)
{
    struct DlTask *t = (struct DlTask *)arg;
    struct McPkgNetBuf buf = { 0 };
    long http = 0;
    int ne;               /* MCPKG_NET_ERROR */
    int fe;               /* MCPKG_FS_ERROR */
    struct McPkgNetDlResult *res = NULL;

    /* 1) perform GET into memory buffer */
    ne = mcpkg_net_request(t->cli, "GET", t->path,
                           (const char *const *)t->query,
                           NULL, 0, &buf, &http);
    if (ne != MCPKG_NET_NO_ERROR) {
        if (out_err) *out_err = ne;
        mcpkg_net_buf_free(&buf);
        free(t->path);
        strv_free(t->query);
        free(t->outfile);
        free(t);
        return 0;
    }

    /* 2) write to file */
    fe = mcpkg_fs_write_all(t->outfile, buf.data, buf.len, /*overwrite*/1);
    if (fe != MCPKG_FS_OK) {
        if (out_err) *out_err = map_fs_to_net(fe);
        mcpkg_net_buf_free(&buf);
        free(t->path);
        strv_free(t->query);
        free(t->outfile);
        free(t);
        return 0;
    }

    /* 3) build result */
    res = (struct McPkgNetDlResult *)calloc(1, sizeof(*res));
    if (!res) {
        if (out_err) *out_err = MCPKG_NET_ERR_NOMEM;
        mcpkg_net_buf_free(&buf);
        free(t->path);
        strv_free(t->query);
        free(t->outfile);
        free(t);
        return 0;
    }
    res->outfile = t->outfile; /* transfer ownership */
    res->http_code = http;
    res->bytes_written = buf.len;

    /* cleanup */
    mcpkg_net_buf_free(&buf);
    free(t->path);
    strv_free(t->query);
    /* t->outfile moved to res */
    free(t);

    if (out_result) *out_result = res;
    if (out_err)    *out_err    = 0;
    return 0;
}

int mcpkg_net_downloader_fetch(struct McPkgNetDownloader *dl,
                               const char *path,
                               const char *const *query_kv_pairs,
                               const char *outfile,
                               struct McPkgThreadFuture **out_future)
{
    struct DlTask *t;
    int rc;
    char *final_out = NULL;

    if (!dl || !dl->cli || !path || !outfile || !out_future)
        return MCPKG_THREAD_E_INVAL;

    /* resolve outfile against download_dir if relative */
    if (path_is_abs(outfile) || !dl->download_dir) {
        final_out = cdup(outfile);
    } else {
        final_out = join_paths(dl->download_dir, outfile);
    }
    if (!final_out)
        return MCPKG_THREAD_E_NOMEM;

    t = (struct DlTask *)calloc(1, sizeof(*t));
    if (!t) {
        free(final_out);
        return MCPKG_THREAD_E_NOMEM;
    }

    t->cli = dl->cli;
    t->path = cdup(path);
    t->outfile = final_out;
    t->query = strv_dup(query_kv_pairs);

    if (!t->path || (query_kv_pairs && !t->query)) {
        strv_free(t->query);
        free(t->outfile);
        free(t->path);
        free(t);
        return MCPKG_THREAD_E_NOMEM;
    }

    rc = mcpkg_thread_pool_call_future(dl->pool, dl_task_run, t, out_future);
    if (rc != MCPKG_THREAD_NO_ERROR) {
        strv_free(t->query);
        free(t->outfile);
        free(t->path);
        free(t);
        return rc;
    }
    return MCPKG_THREAD_NO_ERROR;
}
