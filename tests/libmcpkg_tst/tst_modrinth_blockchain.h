/* SPDX-License-Identifier: MIT */
#ifndef TST_MODRINTH_BLOCKCHAIN_H
#define TST_MODRINTH_BLOCKCHAIN_H

/* stdlib */
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/* test macros */
#include <tst_macros.h>

/* net + modrinth */
#include "net/modrinth/mcpkg_net_modrinth_client.h"

/* parsing (generated API struct helpers) */
#include "net/modrinth/mcpkg_modrinth_json.h"

/* mp-generated debug/free for pkg.meta */
#include "mp/mcpkg_mp_pkg_meta.h"

/* repo-page + crypto */
#include "crypto/mcpkg_repo_page.h"
#include "crypto/mcpkg_leaf_index_cache.h"
#include "crypto/mcpkg_merkle_b2b32.h"
#include "crypto/mcpkg_merkle_consistency_b2b32.h"
#include "mcpkg_crypto_hash.h"

/* MP structs for attestation/block/STH/consistency so we can dereference */
#include "mp/mcpkg_mp_ledger_attestation.h"
#include "mp/mcpkg_mp_ledger_block.h"
#include "mp/mcpkg_mp_ledger_sth.h"
#include "mp/mcpkg_mp_ledger_consistency.h"

/* containers */
#include "container/mcpkg_list.h"
#include "container/mcpkg_str_list.h"

/* ---------- small helpers ---------- */

static void hex32(char out[65], const uint8_t b[32])
{
	static const char *H = "0123456789abcdef";
	size_t i;
	for (i = 0; i < 32; i++) {
		out[i * 2 + 0] = H[(b[i] >> 4) & 0xF];
		out[i * 2 + 1] = H[b[i] & 0xF];
	}
	out[64] = '\0';
}

static void hex64(char out[129], const uint8_t b[64])
{
	static const char *H = "0123456789abcdef";
	size_t i;
	for (i = 0; i < 64; i++) {
		out[i * 2 + 0] = H[(b[i] >> 4) & 0xF];
		out[i * 2 + 1] = H[b[i] & 0xF];
	}
	out[128] = '\0';
}

/* Deterministic toy signer for tests (NOT a real signature) */
static int test_signer_cb(const void *data, size_t len,
                          uint8_t out_sig64[64], uint8_t out_pub32[32],
                          void *ctx)
{
	const char *tag = (const char *)(ctx ? ctx : "nil");
	uint8_t h1[32], h2[32];
	int rc;

	/* pub = H(tag) */
	rc = mcpkg_crypto_blake2b32_buf(tag, strlen(tag), out_pub32);
	CHECK_EQ_INT("test_signer pub err", rc, 0);

	/* h1 = H(tag || data) */
	{
		size_t tn = strlen(tag);
		uint8_t *tmp = (uint8_t *)malloc(tn + len);
		CHECK(tmp != NULL, "alloc tmp");
		memcpy(tmp, tag, tn);
		if (len) memcpy(tmp + tn, data, len);
		rc = mcpkg_crypto_blake2b32_buf(tmp, tn + len, h1);
		CHECK_EQ_INT("test_signer sig half1 err", rc, 0);
		free(tmp);
	}

	/* h2 = H(pub || data) */
	{
		uint8_t *tmp = (uint8_t *)malloc(32 + len);
		CHECK(tmp != NULL, "alloc tmp");
		memcpy(tmp, out_pub32, 32);
		if (len) memcpy(tmp + 32, data, len);
		rc = mcpkg_crypto_blake2b32_buf(tmp, 32 + len, h2);
		CHECK_EQ_INT("test_signer sig half2 err", rc, 0);
		free(tmp);
	}

	memcpy(out_sig64 + 0,  h1, 32);
	memcpy(out_sig64 + 32, h2, 32);
	return 0;
}

/* ---------- the actual test ---------- */

static void test_modrinth_end_to_end(void)
{
#if TST_ONLINE
	const char *base_url   = "https://api.modrinth.com";
	const char *ua         = "mcpkg-e2e/0.1 (+tests)";
	const char *loader     = "fabric";
	const char *mc_version = "1.21.8";
	const int   limit      = 100;
	const int   offset     = 0;
	const size_t cap_hint  = 100;

	McPkgModrinthClientCfg cfg;
	McPkgModrinthClient *mc = NULL;
	McPkgList *pkgs = NULL;
	size_t n = 0, i;

	struct McPkgMerkleB2B32 *tree = NULL;
	struct McPkgLeafIndexCache *lic = NULL;
	struct McPkgRepoPage *page = NULL;

	uint8_t prev[32] = {0};
	struct McPkgSTH *sth = NULL;
	struct McPkgBlock *blk = NULL;
	struct McPkgConsistencyProof *cons = NULL;
	int rc;

	memset(&cfg, 0, sizeof(cfg));
	cfg.base_url   = base_url;
	cfg.user_agent = ua;

	mc = mcpkg_net_modrinth_client_new(&cfg);
	CHECK_NONNULL("modrinth client", mc);

	/* fetch one page of search results for loader+mc_version */
	rc = mcpkg_net_modrinth_fetch_page_build(mc, loader, mc_version, limit, offset,
	                &pkgs);
	CHECK_EQ_INT("fetch_page_build", rc, MCPKG_NET_NO_ERROR);
	CHECK_NONNULL("pkgs", pkgs);

	n = mcpkg_list_size(pkgs);
	CHECK(n > 0, "at least 1 hit");

	/* build crypto state */
	tree = mcpkg_merkle_b2b32_new(cap_hint);
	CHECK_NONNULL("merkle tree", tree);

	lic = mcpkg_leaf_index_cache_new();
	CHECK_NONNULL("leaf-index cache", lic);

	{
		int64_t ts = 1755155029562; /* stable test timestamp */
		rc = mcpkg_repo_page_begin(tree, lic, "modrinth", ts,
		                           &test_signer_cb, (void *)"att",
		                           &test_signer_cb, (void *)"mint",
		                           &page);
		CHECK_EQ_INT("repo_page_begin", rc, MCPKG_RPAGE_NO_ERROR);
		CHECK_NONNULL("page", page);
	}

	/* loop packages -> attest + append -> ALWAYS cache leaf index */
	for (i = 0; i < n; i++) {
		struct McPkgCache *p = NULL;
		struct McPkgAttestation *att = NULL;
		uint64_t li0 = 0;

		(void)mcpkg_list_at(pkgs, i, &p);
		if (!p) continue;

		/* debug dump of the meta */
		{
			char *dbg = mcpkg_mp_pkg_meta_debug_str(p);
			if (dbg) {
				tst_info("pkg[%zu] %s", i, dbg);
				free(dbg);
			}
		}

		/* add (attestation out first, then leaf index) */
		rc = mcpkg_repo_page_add(page, p, &att, &li0);
		CHECK_EQ_INT("repo_page_add", rc, MCPKG_RPAGE_NO_ERROR);

		/* log the attestation we just produced */
		if (att) {
			char hhex[65], pubhex[65], sighex[129];
			hex32(hhex, att->manifest_sha256);
			hex32(pubhex, att->signer_pub);
			hex64(sighex, att->signature);
			tst_info("attest[%zu]: pkg=%s ver=%s ts=%" PRId64 " leaf=%" PRIu64,
			         i, tst_safe_str(att->pkg_id), tst_safe_str(att->version),
			         att->ts_ms, li0);
			tst_info("attest[%zu]: manifest_b2b32=%s", i, hhex);
			tst_info("attest[%zu]: signer_pub=%s", i, pubhex);
			tst_info("attest[%zu]: sig=%s", i, sighex);
			mcpkg_mp_ledger_attestation_free(att);
			att = NULL;
		}
	}

	/* finalize the page -> STH, Block, Consistency proof */
	rc = mcpkg_repo_page_finish(page, prev, 1 /* height */, &sth, &blk, &cons);
	CHECK_EQ_INT("repo_page_finish", rc, MCPKG_RPAGE_NO_ERROR);
	CHECK_NONNULL("sth", sth);
	CHECK_NONNULL("blk", blk);
	CHECK_NONNULL("cons", cons);

	/* pretty-print STH + Block + Consistency */
	{
		char root_hex[65], pub_hex[65], sig_hex[129];
		hex32(root_hex, blk->sth->root);
		hex32(pub_hex, blk->mint_pub);
		hex64(sig_hex, blk->sig);

		tst_info("STH: size=%" PRIu64 " root=%s ts_ms=%" PRIu64 " first=%" PRIu64
		         " last=%" PRIu64,
		         blk->sth->size, root_hex, blk->sth->ts_ms,
		         blk->sth->first, blk->sth->last);
		tst_info("Block: height=%" PRIu64 " mint_pub=%s sig=%s",
		         blk->height, pub_hex, sig_hex);

		{
			size_t cN = (cons && cons->nodes) ? mcpkg_list_size(cons->nodes) : 0;
			tst_info("Consistency: nodes=%zu", cN);
		}
	}

	/* cleanup */
	if (blk)  mcpkg_mp_ledger_block_free(blk);  /* frees blk->sth internally */
	else if (sth) mcpkg_mp_ledger_sth_free(sth);
	if (cons) mcpkg_mp_ledger_consistency_free(cons);

	if (tree) mcpkg_merkle_b2b32_free(tree);
	if (lic)  mcpkg_leaf_index_cache_free(lic);

	if (pkgs) {
		size_t iN = mcpkg_list_size(pkgs), i2;
		for (i2 = 0; i2 < iN; i2++) {
			struct McPkgCache *p = NULL;
			if (mcpkg_list_at(pkgs, i2, &p) == MCPKG_CONTAINER_OK && p)
				mcpkg_mp_pkg_meta_free(p);
		}
		mcpkg_list_free(pkgs);
	}

	if (mc) mcpkg_net_modrinth_client_free(mc);

#else
	tst_info("modrinth e2e: skipped (TST_ONLINE=0)");
#endif
}

/* ---------- runner ---------- */

static inline void run_modrinth_blockchain_e2e(void)
{
	int before = g_tst_fails;
	tst_info("mcpkg modrinth blockchain e2e: starting...");
	test_modrinth_end_to_end();
	if (g_tst_fails == before)
		(void)TST_WRITE(TST_OUT_FD, "mcpkg modrinth blockchain e2e: OK\n", 36);
}

#endif /* TST_MODRINTH_BLOCKCHAIN_H */
