/* SPDX-License-Identifier: MIT */
#ifndef TST_NET_DOWNLOADER_H
#define TST_NET_DOWNLOADER_H

#include <string.h>
#include <stdlib.h>

/* fs */
#include <fs/mcpkg_fs_file.h>
#include <fs/mcpkg_fs_error.h>
/* net */
#include <net/mcpkg_net_client.h>
#include <net/mcpkg_net_downloader.h>
/* threads (for futures) */
#include <threads/mcpkg_thread_future.h>
/* test macros */
#include <tst_macros.h>

/* Full URLs we intend to fetch (for documentation/visibility). */
static const char *k_rfc_urls[10] = {
	"https://www.rfc-editor.org/rfc/rfc9110.txt",
	"https://www.rfc-editor.org/rfc/rfc9111.txt",
	"https://www.rfc-editor.org/rfc/rfc9112.txt",
	"https://www.rfc-editor.org/rfc/rfc3986.txt",
	"https://www.rfc-editor.org/rfc/rfc1951.txt",
	"https://www.rfc-editor.org/rfc/rfc1952.txt",
	"https://www.rfc-editor.org/rfc/rfc7539.txt",
	"https://www.rfc-editor.org/rfc/rfc8446.txt",
	"https://www.rfc-editor.org/rfc/rfc6266.txt",
	"https://www.rfc-editor.org/rfc/rfc5861.txt",
};

/* Paths relative to base_url "https://www.rfc-editor.org" */
static const char *k_rfc_paths[10] = {
	"/rfc/rfc9110.txt",
	"/rfc/rfc9111.txt",
	"/rfc/rfc9112.txt",
	"/rfc/rfc3986.txt",
	"/rfc/rfc1951.txt",
	"/rfc/rfc1952.txt",
	"/rfc/rfc7539.txt",
	"/rfc/rfc8446.txt",
	"/rfc/rfc6266.txt",
	"/rfc/rfc5861.txt",
};

/* Output file names (written into CWD; we unlink after). */
static const char *k_out_names[10] = {
	"dl_rfc9110.txt",
	"dl_rfc9111.txt",
	"dl_rfc9112.txt",
	"dl_rfc3986.txt",
	"dl_rfc1951.txt",
	"dl_rfc1952.txt",
	"dl_rfc7539.txt",
	"dl_rfc8446.txt",
	"dl_rfc6266.txt",
	"dl_rfc5861.txt",
};

static void test_downloader_parallel_10(void)
{
	if (!TST_ONLINE) {
		printf("downloader online test skipped (set MCPKG_TEST_ONLINE=1)\n");
		return;
	}

	struct McPkgNetBuf buf;
	(void)mcpkg_net_buf_init(&buf, 0);

	/* 1) Global net init */
	CHECK_OK_NET("global_init", mcpkg_net_global_init());

	/* 2) Build client on rfc-editor.org */
	McPkgNetClient *cli = NULL;
	{
		McPkgNetClientCfg cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.base_url = "https://www.rfc-editor.org";
		cfg.user_agent = "mcpkg-tests/0.1 (unit)";
		cfg.connect_timeout_ms = 5000;
		cfg.operation_timeout_ms = 20000;

		cli = mcpkg_net_client_new(&cfg);
		CHECK_NONNULL("client_new", cli);
	}

	/* 3) Downloader with internal pool: 10 parallel, q=10 */
	McPkgNetDownloader *dl = NULL;
	{
		struct McPkgNetDownloaderCfg dcfg;
		memset(&dcfg, 0, sizeof(dcfg));
		dcfg.client   = cli;
		dcfg.pool     = NULL;     /* create internal */
		dcfg.parallel = 10;
		dcfg.queue    = 10;
		dcfg.download_dir = NULL; /* write into CWD */

		CHECK_EQ_INT("downloader_new",
		             mcpkg_net_downloader_new(&dcfg, &dl),
		             0);
		CHECK_NONNULL("downloader handle", dl);
	}

	/* 4) Submit 10 downloads at once */
	struct McPkgThreadFuture *f[10] = { 0 };
	for (int i = 0; i < 10; i++) {
		int rc = mcpkg_net_downloader_fetch(dl,
		                                    k_rfc_paths[i],
		                                    NULL, /* no query */
		                                    k_out_names[i],
		                                    &f[i]);
		CHECK_EQ_INT("fetch enqueue rc==0", rc, 0);
		CHECK_NONNULL("future non-null", f[i]);
	}

	/* 5) Wait for all; validate outputs */
	for (int i = 0; i < 10; i++) {
		void *vres = NULL;
		int ferr = -1;

		/* 0 => infinite wait; pick e.g. 60000 for 60s if you prefer bounded */
		int rc = mcpkg_thread_future_wait(f[i], 0, &vres, &ferr);
		CHECK_EQ_INT("future wait rc==0", rc, 0);
		CHECK_EQ_INT("future err==0", ferr, 0);

		{
			struct McPkgNetDlResult *r = (struct McPkgNetDlResult *)vres;
			CHECK_NONNULL("dl result", r);
			CHECK_NONNULL("dl result->outfile", r->outfile);
			CHECK_EQ_INT("http 200", (int)r->http_code, 200);
			CHECK(mcpkg_fs_file_exists(r->outfile) == 1, "file exists");
			CHECK(r->bytes_written > 0, "wrote > 0 bytes");

			(void)mcpkg_fs_unlink(r->outfile);
			free(r->outfile);
			free(r);
		}
	}


	/* 6) Cleanup */
	mcpkg_net_downloader_free(dl);
	mcpkg_net_client_free(cli);
	mcpkg_net_global_cleanup();
	mcpkg_net_buf_free(&buf);
}

/* Runner for this header */
static inline void run_tst_net_downloader(void)
{
	int before = g_tst_fails;
	tst_info("mcpkg net downloader: starting (10 parallel RFCs)...");
	test_downloader_parallel_10();
	if (g_tst_fails == before)
		(void)TST_WRITE(TST_OUT_FD,
		                "mcpkg net downloader: OK\n",
		                27);
}

#endif /* TST_NET_DOWNLOADER_H */
