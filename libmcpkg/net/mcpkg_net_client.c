/* SPDX-License-Identifier: MIT */
#include "net/mcpkg_net_client.h"

#include "mcpkg_export.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <curl/curl.h>

#include "net/mcpkg_net_url.h"
#include "net/mcpkg_net_util.h"   /* McPkgNetBuf + buf_init/reserve/free */

/* ---------------- internal: error mapping ---------------- */

static int curlcode2neterr(CURLcode ce)
{
    switch (ce) {
    case CURLE_OK:                     return MCPKG_NET_NO_ERROR;
    case CURLE_OUT_OF_MEMORY:          return MCPKG_NET_ERR_NOMEM;
    case CURLE_OPERATION_TIMEDOUT:     return MCPKG_NET_ERR_TIMEOUT;
    case CURLE_TOO_MANY_REDIRECTS:     return MCPKG_NET_ERR_PROTO;
    case CURLE_URL_MALFORMAT:
    case CURLE_UNSUPPORTED_PROTOCOL:   return MCPKG_NET_ERR_PROTO;
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_CONNECT:
    case CURLE_SEND_ERROR:
    case CURLE_RECV_ERROR:             return MCPKG_NET_ERR_IO;
    default:                           return MCPKG_NET_ERR_OTHER;
    }
}

/* ---------------- internal: small helpers ---------------- */

static struct curl_slist *slist_dup(const struct curl_slist *in)
{
    const struct curl_slist *p = in;
    struct curl_slist *out = NULL;
    while (p) {
        struct curl_slist *n = curl_slist_append(out, p->data);
        if (!n) {
            curl_slist_free_all(out);
            return NULL;
        }
        out = n;
        p = p->next;
    }
    return out;
}

static int is_abs_url(const char *s)
{
    if (!s) return 0;
    return !strncmp(s, "http://", 7) ||
           !strncmp(s, "https://", 8) ||
           !strncmp(s, "file://", 7);
}

/* lower-case ASCII header key in-place */
static void str_tolower_ascii(char *s)
{
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c >= 'A' && c <= 'Z') *s = (char)(c - 'A' + 'a');
    }
}

/* ---------------- client struct ---------------- */

struct McPkgNetClient {
    McPkgNetUrl             *base;                  /* base URL */
    char                    *user_agent;            /* UA string (optional) */
    struct curl_slist       *headers;               /* default headers */

    long                    connect_timeout_ms;     /* 0 = libcurl default */
    long                    operation_timeout_ms;   /* 0 = libcurl default */

    /* best-effort last-seen rate-limit values (unsynchronized but benign) */
    int                     rl_limit;               /* -1 unknown */
    int                     rl_remaining;           /* -1 unknown */
    int                     rl_reset;               /* -1 unknown (epoch) */
};

/* ---------------- global init/cleanup ---------------- */

MCPKG_API int
mcpkg_net_global_init(void)
{
    CURLcode ce = curl_global_init(CURL_GLOBAL_DEFAULT);
    return (ce == CURLE_OK) ? MCPKG_NET_NO_ERROR : MCPKG_NET_ERR_SYS;
}

MCPKG_API void
mcpkg_net_global_cleanup(void)
{
    curl_global_cleanup();
}

/* ---------------- lifecycle ---------------- */

MCPKG_API McPkgNetClient *
mcpkg_net_client_new(const McPkgNetClientCfg *cfg)
{
    McPkgNetClient *c = NULL;

    if (!cfg || !cfg->base_url)
        return NULL;

    c = (McPkgNetClient *)calloc(1, sizeof(*c));
    if (!c)
        return NULL;

    /* base URL */
    c->base = mcpkg_net_url_new();
    if (!c->base) {
        free(c);
        return NULL;
    }
    if (mcpkg_net_url_parse(c->base, cfg->base_url) != MCPKG_NET_NO_ERROR) {
        mcpkg_net_url_free(c->base);
        free(c);
        return NULL;
    }

    /* user-agent */
    if (cfg->user_agent && cfg->user_agent[0]) {
        size_t n = strlen(cfg->user_agent) + 1U;
        c->user_agent = (char *)malloc(n);
        if (!c->user_agent) {
            mcpkg_net_url_free(c->base);
            free(c);
            return NULL;
        }
        memcpy(c->user_agent, cfg->user_agent, n);
    }

    c->connect_timeout_ms   = (long)cfg->connect_timeout_ms;
    c->operation_timeout_ms = (long)cfg->operation_timeout_ms;

    c->rl_limit = c->rl_remaining = c->rl_reset = -1;
    return c;
}

MCPKG_API void
mcpkg_net_client_free(McPkgNetClient *c)
{
    if (!c) return;
    if (c->headers)
        curl_slist_free_all(c->headers);
    if (c->user_agent)
        free(c->user_agent);
    if (c->base)
        mcpkg_net_url_free(c->base);
    free(c);
}

/* ---------------- configuration ---------------- */

MCPKG_API int
mcpkg_net_client_set_header(McPkgNetClient *c, const char *header_line)
{
    if (!c || !header_line)
        return MCPKG_NET_ERR_INVALID;

    struct curl_slist *n = curl_slist_append(c->headers, header_line);
    if (!n)
        return MCPKG_NET_ERR_NOMEM;
    c->headers = n;
    return MCPKG_NET_NO_ERROR;
}

MCPKG_API int
mcpkg_net_client_set_timeout(McPkgNetClient *c,
                             long connect_timeout_ms,
                             long operation_timeout_ms)
{
    if (!c)
        return MCPKG_NET_ERR_INVALID;
    c->connect_timeout_ms   = connect_timeout_ms;
    c->operation_timeout_ms = operation_timeout_ms;
    return MCPKG_NET_NO_ERROR;
}

MCPKG_API McPkgNetRateLimit
mcpkg_net_get_ratelimit(McPkgNetClient *c)
{
    McPkgNetRateLimit rl = { -1, -1, -1 };
    if (!c) return rl;
    rl.limit     = c->rl_limit;
    rl.remaining = c->rl_remaining;
    rl.reset     = c->rl_reset;
    return rl;
}

/* ---------------- request plumbing ---------------- */

static size_t curl_write_cb(char *ptr, size_t size, size_t nmemb, void *ud)
{
    struct McPkgNetBuf *b = (struct McPkgNetBuf *)ud;
    size_t n = size * nmemb;

    if (!b || (n && !ptr))
        return 0;

    if (mcpkg_net_buf_reserve(b, b->len + n) != MCPKG_NET_NO_ERROR)
        return 0;

    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    return n;
}

/* Capture a few rate-limit headers if present (best-effort). */
static size_t curl_header_cb(char *buf, size_t size, size_t nmemb, void *ud)
{
    McPkgNetClient *c = (McPkgNetClient *)ud;
    size_t n = size * nmemb;

    if (!c || n < 4) return n;

    char *colon = memchr(buf, ':', n);
    if (!colon) return n;

    size_t klen = (size_t)(colon - buf);
    if (klen == 0 || klen >= 64) return n;

    char key[64];
    memcpy(key, buf, klen);
    key[klen] = '\0';
    str_tolower_ascii(key);

    size_t off = (size_t)(colon - buf) + 1;
    while (off < n && (buf[off] == ' ' || buf[off] == '\t')) off++;

    char val[128];
    size_t vlen = 0;
    while (off + vlen < n && vlen + 1 < sizeof(val)) {
        char ch = buf[off + vlen];
        if (ch == '\r' || ch == '\n') break;
        val[vlen++] = ch;
    }
    val[vlen] = '\0';

    if (strcmp(key, "x-ratelimit-limit") == 0 ||
        strcmp(key, "ratelimit-limit") == 0) {
        c->rl_limit = atoi(val);
    } else if (strcmp(key, "x-ratelimit-remaining") == 0 ||
               strcmp(key, "ratelimit-remaining") == 0) {
        c->rl_remaining = atoi(val);
    } else if (strcmp(key, "x-ratelimit-reset") == 0 ||
               strcmp(key, "ratelimit-reset") == 0) {
        c->rl_reset = atoi(val);
    }

    return n;
}

/* Build a concrete URL string into a malloc'd buffer; caller frees *out_url. */
static int build_request_url(McPkgNetClient *c,
                             const char *path_or_abs,
                             const char *const *query_kv_pairs,
                             char **out_url)
{
    int ret = MCPKG_NET_NO_ERROR;
    char *url = NULL;

    if (!c || !c->base || !out_url)
        return MCPKG_NET_ERR_INVALID;

    if (is_abs_url(path_or_abs)) {
        size_t n = strlen(path_or_abs) + 1U;
        url = (char *)malloc(n);
        if (!url) return MCPKG_NET_ERR_NOMEM;
        memcpy(url, path_or_abs, n);
        *out_url = url;
        return MCPKG_NET_NO_ERROR;
    }

    /* work on a clone of the base */
    {
        McPkgNetUrl *u = mcpkg_net_url_clone(c->base);
        if (!u) return MCPKG_NET_ERR_NOMEM;

        if (path_or_abs && path_or_abs[0]) {
            ret = mcpkg_net_url_set_path(u, path_or_abs);
            if (ret != MCPKG_NET_NO_ERROR) {
                mcpkg_net_url_free(u);
                return ret;
            }
        }

        if (query_kv_pairs && query_kv_pairs[0]) {
            /* join k,v pairs into "k=v&k=v" (assumes already encoded) */
            size_t cap = 1, i;
            for (i = 0; query_kv_pairs[i]; i += 2) {
                const char *k = query_kv_pairs[i];
                const char *v = query_kv_pairs[i + 1];
                cap += (k ? strlen(k) : 0) + 1 + (v ? strlen(v) : 0) + 1;
            }
            char *q = (char *)malloc(cap ? cap : 1);
            if (!q) {
                mcpkg_net_url_free(u);
                return MCPKG_NET_ERR_NOMEM;
            }
            q[0] = '\0';
            for (i = 0; query_kv_pairs[i]; i += 2) {
                if (i) strcat(q, "&");
                strcat(q, query_kv_pairs[i]);
                strcat(q, "=");
                if (query_kv_pairs[i + 1]) strcat(q, query_kv_pairs[i + 1]);
            }
            ret = mcpkg_net_url_set_query(u, q);
            free(q);
            if (ret != MCPKG_NET_NO_ERROR) {
                mcpkg_net_url_free(u);
                return ret;
            }
        }

        /* stringify into malloc'd string */
        ret = mcpkg_net_url_to_string(u, &url);
        mcpkg_net_url_free(u);
        if (ret != MCPKG_NET_NO_ERROR)
            return ret;
    }

    *out_url = url;
    return MCPKG_NET_NO_ERROR;
}

/* ---------------- public request API ---------------- */

MCPKG_API int
mcpkg_net_request(McPkgNetClient *c,
                  const char *method,
                  const char *path_or_abs,
                  const char *const *query_kv_pairs,
                  const void *in_body, size_t in_len,
                  struct McPkgNetBuf *out_body,
                  long *out_http)
{
    int ret = MCPKG_NET_NO_ERROR;
    CURL *eh = NULL;
    char *url = NULL;
    struct curl_slist *hdr = NULL;
    long http = 0;

    if (!c || !method || !out_body)
        return MCPKG_NET_ERR_INVALID;

    ret = mcpkg_net_buf_init(out_body, 0);
    if (ret != MCPKG_NET_NO_ERROR)
        return ret;

    ret = build_request_url(c, path_or_abs, query_kv_pairs, &url);
    if (ret != MCPKG_NET_NO_ERROR) {
        mcpkg_net_buf_free(out_body);
        return ret;
    }

    eh = curl_easy_init();
    if (!eh) {
        free(url);
        mcpkg_net_buf_free(out_body);
        return MCPKG_NET_ERR_SYS;
    }

    /* duplicate headers for this request */
    if (c->headers) {
        hdr = slist_dup(c->headers);
        if (!hdr) {
            curl_easy_cleanup(eh);
            free(url);
            mcpkg_net_buf_free(out_body);
            return MCPKG_NET_ERR_NOMEM;
        }
        curl_easy_setopt(eh, CURLOPT_HTTPHEADER, hdr);
    }

    curl_easy_setopt(eh, CURLOPT_URL, url);
    curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(eh, CURLOPT_NOSIGNAL, 1L); /* thread-safe on POSIX */
    if (c->user_agent && c->user_agent[0])
        curl_easy_setopt(eh, CURLOPT_USERAGENT, c->user_agent);
    if (c->connect_timeout_ms > 0)
        curl_easy_setopt(eh, CURLOPT_CONNECTTIMEOUT_MS, (long)c->connect_timeout_ms);
    if (c->operation_timeout_ms > 0)
        curl_easy_setopt(eh, CURLOPT_TIMEOUT_MS, (long)c->operation_timeout_ms);

    /* method + body */
    if (!strcmp(method, "GET")) {
        curl_easy_setopt(eh, CURLOPT_HTTPGET, 1L);
    } else if (!strcmp(method, "HEAD")) {
        curl_easy_setopt(eh, CURLOPT_NOBODY, 1L);
    } else if (!strcmp(method, "POST")) {
        curl_easy_setopt(eh, CURLOPT_POST, 1L);
        if (in_body && in_len) {
            curl_easy_setopt(eh, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)in_len);
            curl_easy_setopt(eh, CURLOPT_POSTFIELDS, (const char *)in_body);
        }
    } else {
        /* custom / others (PUT/PATCH/DELETE...) */
        curl_easy_setopt(eh, CURLOPT_CUSTOMREQUEST, method);
        if (in_body && in_len) {
            curl_easy_setopt(eh, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)in_len);
            curl_easy_setopt(eh, CURLOPT_POSTFIELDS, (const char *)in_body);
        }
    }

    /* body + header callbacks */
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, &curl_write_cb);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)out_body);

    curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, &curl_header_cb);
    curl_easy_setopt(eh, CURLOPT_HEADERDATA, (void *)c);

    {
        CURLcode ce = curl_easy_perform(eh);
        if (ce != CURLE_OK) {
            ret = curlcode2neterr(ce);
            goto done;
        }
    }

    (void)curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http);
    if (out_http) *out_http = http;

done:
    if (hdr) curl_slist_free_all(hdr);
    if (eh)  curl_easy_cleanup(eh);
    if (url) free(url);

    if (ret != MCPKG_NET_NO_ERROR) {
        mcpkg_net_buf_free(out_body);
    }
    return ret;
}
