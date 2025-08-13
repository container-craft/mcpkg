#ifndef TST_NET_H
#define TST_NET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

	ret = mcpkg_net_url_new(&u);
	CHECK_OK_NET("url_new", ret);

	/* is_empty initially */
	ret = mcpkg_net_url_is_empty(u, &flag);
	CHECK_OK_NET("url_is_empty()", ret);
	CHECK_EQ_INT("url_is_empty initial", flag, 1);

	/* set a base URL */
	ret = mcpkg_net_url_set_url(u, "https://example.com/base");
	CHECK_OK_NET("url_set_url", ret);

	/* scheme, host, path */
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

	/* no query/fragment yet */
	flag = -1;
	ret = mcpkg_net_url_has_query(u, &flag);
	CHECK_OK_NET("url_has_query (none)", ret);
	CHECK_EQ_INT("url_has_query==0", flag, 0);

	flag = -1;
	ret = mcpkg_net_url_has_fragment(u, &flag);
	CHECK_OK_NET("url_has_fragment (none)", ret);
	CHECK_EQ_INT("url_has_fragment==0", flag, 0);

	/* set query + fragment */
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

	/* port set/clear */
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

	/* host IDN: UTF-8 host should round-trip; ASCII may be punycode or UTF-8 depending on build */
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
		/* Accept either punycode or same UTF-8 if IDN not enabled */
		ok = (strcmp(ascii, "bücher.de") == 0) ||
		     (strncmp(ascii, "xn--", 4) == 0);
		CHECK(ok, "host_ascii punycode or same got \"%s\"", ascii);
	}

	/* to_string should produce a full URL */
	{
		char full[1024];
		memset(full, 0, sizeof(full));
		ret = mcpkg_net_url_to_string(u, full, sizeof(full));
		CHECK_OK_NET("url_to_string", ret);
		CHECK(full[0] != 0, "url_to_string non-empty");
	}

	/* clear → empty */
	ret = mcpkg_net_url_clear(u);
	CHECK_OK_NET("url_clear", ret);
	flag = -1;
	ret = mcpkg_net_url_is_empty(u, &flag);
	CHECK_OK_NET("url_is_empty after clear", ret);
	CHECK_EQ_INT("url_is_empty==1", flag, 1);

	mcpkg_net_url_free(u);
}

/* ---------- CLIENT TESTS (offline path) ---------- */

static void test_client(void)
{
	int ret = MCPKG_NET_NO_ERROR;
	McPkgNetClient *c = NULL;
	struct McPkgNetBuf body;
	long http = 0;
	FILE *fp = NULL;
	const char *tmpfile = "mcpkg_net_test_offline.bin";

	ret = mcpkg_net_global_init();
	CHECK_OK_NET("global_init", ret);

	ret = mcpkg_net_client_new(&c, "https://api.example.com/v2",
	                           "mcpkg-tests/0.1 (unit)");
	CHECK_OK_NET("client_new", ret);

	ret = mcpkg_net_set_header(c, "Content-Type: application/octet-stream");
	CHECK_OK_NET("set_header", ret);

	ret = mcpkg_net_set_timeout(c, 2000, 5000);
	CHECK_OK_NET("set_timeout", ret);

	ret = mcpkg_net_set_offline_root(c, ".");
	CHECK_OK_NET("set_offline_root", ret);

	ret = mcpkg_net_set_offline(c, 1);
	CHECK_OK_NET("set_offline on", ret);

	/* Prepare offline file */
	fp = fopen(tmpfile, "wb");
	if (fp) {
		const char *data = "hello-net-offline";
		(void)fwrite(data, 1, strlen(data), fp);
		fclose(fp);
	} else {
		tst_fail(PRETTY_FUNC, __FILE__, "failed to create offline test file");
	}

	/* Init output buffer and perform GET */
	ret = mcpkg_net_buf_init(&body, 0);
	CHECK_OK_NET("buf_init", ret);

	ret = mcpkg_net_request(c,
	                        MCPKG_NET_METHOD_GET,
	                        tmpfile,
	                        NULL,
	                        NULL, 0,
	                        &body,
	                        &http);
	CHECK_OK_NET("request offline GET", ret);
	CHECK_EQ_INT("http code 200 (offline)", (int)http, 200);
	CHECK(body.len > 0, "offline body has content");

	/* Rate limit should be unknown in offline */
	{
		McPkgNetRateLimit rl;
		ret = mcpkg_net_get_ratelimit(c, &rl);
		CHECK_OK_NET("get_ratelimit", ret);
		CHECK_EQ_INT("rl.limit == -1", (int)rl.limit, -1);
		CHECK_EQ_INT("rl.remaining == -1", (int)rl.remaining, -1);
		CHECK_EQ_INT("rl.reset == -1", (int)rl.reset, -1);
	}

	mcpkg_net_buf_free(&body);

	/* Cleanup */
	(void)remove(tmpfile);
	mcpkg_net_client_free(c);
	mcpkg_net_global_cleanup();
}

static inline void run_tst_net(void)
{
	int before = g_tst_fails;
	tst_info("mcpkg networking tests: starting...");
	test_url();
	test_client();
	if (g_tst_fails == before)
		(void)TST_WRITE(TST_OUT_FD, "mcpkg networking tests: OK\n", 31);
}

#endif /* TST_NET_H */
