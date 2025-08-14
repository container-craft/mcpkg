/* SPDX-License-Identifier: MIT */
#ifndef TST_NET_H
#define TST_NET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#if defined(_WIN32)
#  include <direct.h>
#  define getcwd _getcwd
#else
#  include <unistd.h>
#endif

/* fs module */
#include <fs/mcpkg_fs_file.h>
#include <fs/mcpkg_fs_error.h>
/* net module */
#include <net/mcpkg_net_client.h>
#include <net/mcpkg_net_url.h>
#include <net/mcpkg_net_util.h>
/* test macros */
#include <tst_macros.h>

/* ---------- URL TESTS ---------- */

static void test_url(void)
{
    int ret = MCPKG_NET_NO_ERROR;
    McPkgNetUrl *u = NULL;
    char buf[512];
    int flag = 0;
    int port = 0;

    u = mcpkg_net_url_new();
    CHECK_NONNULL("url_new", u);

    ret = mcpkg_net_url_is_empty(u, &flag);
    CHECK_OK_NET("url_is_empty()", ret);
    CHECK_EQ_INT("url_is_empty initial", flag, 1);

    ret = mcpkg_net_url_parse(u, "https://example.com/base");
    CHECK_OK_NET("url_parse", ret);

    memset(buf, 0, sizeof(buf));
    ret = mcpkg_net_url_scheme(u, buf, sizeof(buf));
    CHECK_OK_NET("url_scheme get", ret);
    CHECK_STR("url_scheme==https", buf, "https");

    memset(buf, 0, sizeof(buf));
    ret = mcpkg_net_url_host(u, buf, sizeof(buf));
    CHECK_OK_NET("url_host get", ret);
    CHECK_STR("url_host==example.com", buf, "example.com");

    memset(buf, 0, sizeof(buf));
    ret = mcpkg_net_url_path(u, buf, sizeof(buf));
    CHECK_OK_NET("url_path get", ret);
    CHECK_STR("url_path==/base", buf, "/base");

    flag = -1;
    ret = mcpkg_net_url_has_query(u, &flag);
    CHECK_OK_NET("url_has_query (none)", ret);
    CHECK_EQ_INT("url_has_query==0", flag, 0);

    flag = -1;
    ret = mcpkg_net_url_has_fragment(u, &flag);
    CHECK_OK_NET("url_has_fragment (none)", ret);
    CHECK_EQ_INT("url_has_fragment==0", flag, 0);

    ret = mcpkg_net_url_set_query(u, "a=1&b=2");
    CHECK_OK_NET("url_set_query", ret);
    ret = mcpkg_net_url_has_query(u, &flag);
    CHECK_OK_NET("url_has_query (set)", ret);
    CHECK_EQ_INT("url_has_query==1", flag, 1);

    memset(buf, 0, sizeof(buf));
    ret = mcpkg_net_url_query(u, buf, sizeof(buf));
    CHECK_OK_NET("url_query get", ret);
    CHECK_STR("url_query==a=1&b=2", buf, "a=1&b=2");

    ret = mcpkg_net_url_set_fragment(u, "sec");
    CHECK_OK_NET("url_set_fragment", ret);
    ret = mcpkg_net_url_has_fragment(u, &flag);
    CHECK_OK_NET("url_has_fragment (set)", ret);
    CHECK_EQ_INT("url_has_fragment==1", flag, 1);

    memset(buf, 0, sizeof(buf));
    ret = mcpkg_net_url_fragment(u, buf, sizeof(buf));
    CHECK_OK_NET("url_fragment get", ret);
    CHECK_STR("url_fragment==sec", buf, "sec");

    ret = mcpkg_net_url_port(u, &port);
    CHECK_OK_NET("url_port (unset)", ret);
    CHECK_EQ_INT("url_port==0 (unset)", port, 0);

    ret = mcpkg_net_url_set_port(u, 8443);
    CHECK_OK_NET("url_set_port 8443", ret);
    port = 0;
    ret = mcpkg_net_url_port(u, &port);
    CHECK_OK_NET("url_port (set)", ret);
    CHECK_EQ_INT("url_port==8443", port, 8443);

    ret = mcpkg_net_url_set_port(u, 0);
    CHECK_OK_NET("url_set_port 0 (clear)", ret);
    port = -1;
    ret = mcpkg_net_url_port(u, &port);
    CHECK_OK_NET("url_port (cleared)", ret);
    CHECK_EQ_INT("url_port==0", port, 0);

    ret = mcpkg_net_url_set_host(u, "bücher.de");
    CHECK_OK_NET("url_set_host (IDN UTF-8)", ret);

    memset(buf, 0, sizeof(buf));
    ret = mcpkg_net_url_host(u, buf, sizeof(buf));
    CHECK_OK_NET("url_host UTF-8 get", ret);
    CHECK_STR("url_host UTF-8 roundtrip", buf, "bücher.de");

    {
        char ascii[512];
        int ok = 0;
        memset(ascii, 0, sizeof(ascii));
        ret = mcpkg_net_url_host_ascii(u, ascii, sizeof(ascii));
        CHECK_OK_NET("url_host_ascii get", ret);
        ok = (strcmp(ascii, "bücher.de") == 0) || (strncmp(ascii, "xn--", 4) == 0);
        CHECK(ok, "host_ascii punycode or same got \"%s\"", ascii);
    }

    {
        char full[1024];
        memset(full, 0, sizeof(full));
        ret = mcpkg_net_url_to_string_buf(u, full, sizeof(full));
        CHECK_OK_NET("url_to_string_buf", ret);
        CHECK(full[0] != 0, "url_to_string non-empty");
    }

    ret = mcpkg_net_url_clear(u);
    CHECK_OK_NET("url_clear", ret);
    flag = -1;
    ret = mcpkg_net_url_is_empty(u, &flag);
    CHECK_OK_NET("url_is_empty after clear", ret);
    CHECK_EQ_INT("url_is_empty==1", flag, 1);

    mcpkg_net_url_free(u);
}

/* Convert FS path → file-URL path. */
static void fs_path_to_file_url_path(char *dst, size_t dstsz, const char *fs)
{
#if defined(_WIN32)
    size_t i, j = 0;
    size_t n = strlen(fs);
    if (n >= 2 && ((fs[1] == ':') &&
                   ((fs[0] >= 'A' && fs[0] <= 'Z') || (fs[0] >= 'a' && fs[0] <= 'z')))) {
        if (j + 1 < dstsz) dst[j++] = '/';
    }
    for (i = 0; i < n && j + 1 < dstsz; i++) {
        char ch = fs[i];
        if (ch == '\\') ch = '/';
        dst[j++] = ch;
    }
    dst[j] = '\0';
#else
    snprintf(dst, dstsz, "%s", fs);
#endif
}

/* ---------- CLIENT TESTS: offline via FS + file:// ---------- */

static void test_client_offline_file(void)
{
    McPkgNetClient *c = NULL;
    struct McPkgNetBuf body;
    long http = -1;
    char cwd[PATH_MAX];
    char fs_full[PATH_MAX * 2];
    char url_path[PATH_MAX * 2];
    const char *tmpfile = "mcpkg_net_test_offline.bin";
    const char *payload = "hello-net-offline";

    /* write test file using FS module */
    CHECK_OKFS("fs write tmp",
               mcpkg_fs_write_all(tmpfile, payload, (size_t)strlen(payload), /*overwrite*/1));

    memset(cwd, 0, sizeof(cwd));
    (void)getcwd(cwd, sizeof(cwd) - 1);
    snprintf(fs_full, sizeof(fs_full), "%s/%s", cwd, tmpfile);
    fs_path_to_file_url_path(url_path, sizeof(url_path), fs_full);

    CHECK_OK_NET("global_init", mcpkg_net_global_init());

    {
        McPkgNetClientCfg cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.base_url = "file:///";	/* parser wants 'file:///' */
        cfg.user_agent = "mcpkg-tests/0.1 (unit)";
        cfg.connect_timeout_ms = 2000;
        cfg.operation_timeout_ms = 5000;

        c = mcpkg_net_client_new(&cfg);
        CHECK_NONNULL("client_new", c);
    }

    CHECK_OK_NET("set_header",
                 mcpkg_net_client_set_header(c, "Content-Type: application/octet-stream"));
    CHECK_OK_NET("set_timeout",
                 mcpkg_net_client_set_timeout(c, 2000, 5000));

    CHECK_OK_NET("buf_init", mcpkg_net_buf_init(&body, 0));

    {
        int rc = mcpkg_net_request(c,
                                   "GET",
                                   url_path,
                                   NULL,
                                   NULL, 0,
                                   &body,
                                   &http);
        CHECK_OK_NET("request file:// GET", rc);
        CHECK(body.len > 0, "offline body has content");
        CHECK(http == 0 || http == 200, "file:// code acceptable got=%ld", http);
    }

    mcpkg_net_buf_free(&body);

    /* cleanup via FS module */
    CHECK_OKFS("fs unlink tmp", mcpkg_fs_unlink(tmpfile));

    mcpkg_net_client_free(c);
    mcpkg_net_global_cleanup();
}

/* ---------- CLIENT TESTS: small online GET (opt-in) ---------- */
/* Set MCPKG_TEST_ONLINE=1 in env to run this test. */

static void test_client_online(void)
{
    if (!TST_ONLINE) {
        printf("online test skipped (set MCPKG_TEST_ONLINE=1 to enable\n");
        return;
    }

    McPkgNetClient *c = NULL;
    struct McPkgNetBuf body;
    long http = -1;
    int rc;

    CHECK_OK_NET("global_init", mcpkg_net_global_init());

    {
        McPkgNetClientCfg cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.base_url = "https://example.com";
        cfg.user_agent = "mcpkg-tests/0.1 (unit)";
        cfg.connect_timeout_ms = 5000;
        cfg.operation_timeout_ms = 10000;

        c = mcpkg_net_client_new(&cfg);
        CHECK_NONNULL("client_new (online)", c);
    }

    CHECK_OK_NET("buf_init", mcpkg_net_buf_init(&body, 0));

    rc = mcpkg_net_request(c, "GET", "/", NULL, NULL, 0, &body, &http);
    CHECK_OK_NET("request https://example.com/", rc);
    CHECK_EQ_INT("http 200", (int)http, 200);
    CHECK(body.len > 0, "online body has content");

    /* look for a friendly marker; do not require exact HTML */
    {
        int ok = 0;
        const char *needle1 = "Example Domain";
        const char *needle2 = "example";
        size_t i, n1 = strlen(needle1), n2 = strlen(needle2);
        for (i = 0; i + n1 <= body.len; i++) {
            if (memcmp(body.data + i, needle1, n1) == 0) { ok = 1; break; }
        }
        if (!ok) {
            for (i = 0; i + n2 <= body.len; i++) {
                if (memcmp(body.data + i, needle2, n2) == 0) { ok = 1; break; }
            }
        }
        CHECK(ok, "online content looks OK");
    }

    mcpkg_net_buf_free(&body);
    mcpkg_net_client_free(c);
    mcpkg_net_global_cleanup();
}

/* ---------- runner ---------- */

static inline void run_tst_net(void)
{
    int before = g_tst_fails;
    tst_info("mcpkg networking tests: starting...");
    test_url();
    test_client_offline_file();
    test_client_online();
    if (g_tst_fails == before)
        (void)TST_WRITE(TST_OUT_FD, "mcpkg networking tests: OK\n", 31);
}

#endif /* TST_NET_H */
