/* SPDX-License-Identifier: MIT */
#include "crypto/mcpkg_repo_page.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "mcpkg_crypto_hash.h"

#include "crypto/mcpkg_merkle_b2b32.h"
#include "crypto/mcpkg_merkle_consistency_b2b32.h"
#include "crypto/mcpkg_manifest_and_sig.h"
#include "crypto/mcpkg_leaf_index_cache.h"

#include "mcpkg_mp_util.h"
#include "mp/mcpkg_mp_pkg_meta.h"
#include "mp/mcpkg_mp_pkg_origin.h"
#include "mp/mcpkg_mp_ledger_attestation.h"
#include "mp/mcpkg_mp_ledger_sth.h"
#include "mp/mcpkg_mp_ledger_block.h"
#include "mp/mcpkg_mp_ledger_consistency.h"

/* page state */
struct McPkgRepoPage {
	struct McPkgMerkleB2B32         *tree;		/* borrowed */
	struct McPkgLeafIndexCache      *lic;		    /* borrowed */
	const char                      *provider;	    /* borrowed */
	int64_t                         ts_ms;
	uint64_t                        start_size;   	/* for consistency proof */
	mcpkg_signer_cb                 att_signer;
	void                            *att_ctx;
	mcpkg_signer_cb                 mint_signer;
	void                            *mint_ctx;
};

/* ---- helpers ---- */

static void u64le(uint8_t out[8], uint64_t v)
{
	out[0] = (uint8_t)(v);
	out[1] = (uint8_t)(v >> 8);
	out[2] = (uint8_t)(v >> 16);
	out[3] = (uint8_t)(v >> 24);
	out[4] = (uint8_t)(v >> 32);
	out[5] = (uint8_t)(v >> 40);
	out[6] = (uint8_t)(v >> 48);
	out[7] = (uint8_t)(v >> 56);
}

/* build block-sign payload: "MCPKG-BLK\0" || height || prev || sth fields */
static int build_block_sig_payload(const struct McPkgSTH *sth,
                                   uint64_t height,
                                   const uint8_t prev32[32],
                                   uint8_t **out, size_t *out_len)
{
	static const char tag[] = "MCPKG-BLK\0";
	uint8_t *b;
	size_t n = sizeof(tag) + 8 + 32 + 32 + 8 + 8 + 8 + 8;

	if (!sth || !prev32 || !out || !out_len)
		return MCPKG_RPAGE_ERR_INVALID;

	b = (uint8_t *)malloc(n);
	if (!b)
		return MCPKG_RPAGE_ERR_NO_MEMORY;

	{
		size_t off = 0;
		uint8_t tmp[8];

		memcpy(b + off, tag, sizeof(tag));
		off += sizeof(tag);

		u64le(tmp, height);
		memcpy(b + off, tmp, 8);
		off += 8;

		memcpy(b + off, prev32, 32);
		off += 32;

		memcpy(b + off, sth->root, 32);
		off += 32;

		u64le(tmp, (uint64_t)sth->size);
		memcpy(b + off, tmp, 8);
		off += 8;
		u64le(tmp, (uint64_t)sth->ts_ms);
		memcpy(b + off, tmp, 8);
		off += 8;
		u64le(tmp, (uint64_t)sth->first);
		memcpy(b + off, tmp, 8);
		off += 8;
		u64le(tmp, (uint64_t)sth->last);
		memcpy(b + off, tmp, 8);
		off += 8;
	}

	*out = b;
	*out_len = n;
	return MCPKG_RPAGE_NO_ERROR;
}

/* ---- API ---- */

MCPKG_API int
mcpkg_repo_page_begin(struct McPkgMerkleB2B32 *tree,
                      struct McPkgLeafIndexCache *lic,
                      const char *provider,
                      int64_t ts_ms,
                      mcpkg_signer_cb att_signer, void *att_ctx,
                      mcpkg_signer_cb mint_signer, void *mint_ctx,
                      struct McPkgRepoPage **out_pg)
{
	struct McPkgRepoPage *pg;

	if (!out_pg || !tree || !lic || !provider || !att_signer || !mint_signer)
		return MCPKG_RPAGE_ERR_INVALID;

	pg = (struct McPkgRepoPage *)calloc(1, sizeof(*pg));
	if (!pg)
		return MCPKG_RPAGE_ERR_NO_MEMORY;

	pg->tree        = tree;
	pg->lic         = lic;
	pg->provider    = provider;
	pg->ts_ms       = ts_ms;
	pg->start_size  = tree->size;

	pg->att_signer  = att_signer;
	pg->att_ctx     = att_ctx;
	pg->mint_signer = mint_signer;
	pg->mint_ctx    = mint_ctx;

	*out_pg = pg;
	return MCPKG_RPAGE_NO_ERROR;
}

MCPKG_API int
mcpkg_repo_page_add(struct McPkgRepoPage *pg,
                    const struct McPkgCache *meta,
                    struct McPkgAttestation **out_att,
                    uint64_t *out_leaf_index)
{
	void *packed = NULL;
	size_t packed_len = 0;
	uint8_t manifest_b2b32[32];
	uint64_t idx0 = 0;
	struct McPkgAttestation *att = NULL;
	int rc;

	if (!pg || !pg->tree || !meta || !meta->id || !meta->version)
		return MCPKG_RPAGE_ERR_INVALID;

	rc = mcpkg_mp_pkg_meta_pack(meta, &packed, &packed_len);
	if (rc != MCPKG_MP_NO_ERROR)
		return MCPKG_RPAGE_ERR_MP;

	if (mcpkg_crypto_blake2b32_buf(packed, packed_len, manifest_b2b32) != 0) {
		free(packed);
		return MCPKG_RPAGE_ERR_CRYPTO;
	}
	free(packed);

	if (mcpkg_merkle_b2b32_append(pg->tree, manifest_b2b32, &idx0) != 0)
		return MCPKG_RPAGE_ERR_STATE;

	rc = mcpkg_manifest_attest_b2b32(meta->id, meta->version,
	                                 manifest_b2b32, pg->ts_ms,
	                                 pg->att_signer, pg->att_ctx, &att);
	if (rc != MCPKG_RPAGE_NO_ERROR)
		return rc;

	/* leaf index cache: provider:project_id[:version_id] */
	if (pg->lic) {
		const char *proj = NULL, *verid = NULL;
		char *key = NULL;

		if (meta->origin) {
			proj  = meta->origin->project_id;
			verid = meta->origin->version_id;
		}
		if (!proj || !proj[0]) proj = meta->id;

		if (mcpkg_leaf_index_key_from_origin(pg->provider, proj, verid,
		                                     &key) == MCPKG_LIC_NO_ERROR && key) {
			(void)mcpkg_leaf_index_cache_set(pg->lic, key, idx0);
			free(key);
		}
	}

	if (out_att) *out_att = att;
	else mcpkg_mp_ledger_attestation_free(att);

	if (out_leaf_index) *out_leaf_index = idx0;

	return MCPKG_RPAGE_NO_ERROR;
}

MCPKG_API int
mcpkg_repo_page_finish(struct McPkgRepoPage *pg,
                       const uint8_t prev32[32],
                       uint64_t block_height,
                       struct McPkgSTH **out_sth,
                       struct McPkgBlock **out_blk,
                       struct McPkgConsistencyProof **out_cons)
{
	struct McPkgSTH *sth = NULL;
	struct McPkgBlock *blk = NULL;
	struct McPkgConsistencyProof *cons = NULL;
	uint64_t n, m;
	uint8_t root[32];
	uint8_t mint_pub[32], blk_sig[64];
	int rc;

	if (!pg || !pg->tree || !out_sth || !out_blk || !out_cons)
		return MCPKG_RPAGE_ERR_INVALID;

	*out_sth = NULL;
	*out_blk = NULL;
	*out_cons = NULL;

	/* sizes */
	n = pg->tree->size;
	m = pg->start_size; /* snapshot from begin() */

	if (n == 0)
		return MCPKG_RPAGE_ERR_RANGE;

	/* compute root of the current tree */
	rc = mcpkg_merkle_b2b32_root(pg->tree, root);
	if (rc != 0)
		return MCPKG_RPAGE_ERR_CRYPTO;

	/* --- STH --- */
	sth = mcpkg_mp_ledger_sth_new();
	if (!sth) {
		rc = MCPKG_RPAGE_ERR_NO_MEMORY;
		goto fail;
	}
	sth->size  = n;
	memcpy(sth->root, root, 32);
	sth->ts_ms = (uint64_t)pg->ts_ms;
	sth->first = (m > 0) ? m : 0;
	sth->last  = (n > 0) ? (n - 1) : 0;

	/* --- Block (signed) --- */
	rc = mcpkg_manifest_block_sign_b2b32(prev32,
	                                     sth,
	                                     pg->mint_signer, pg->mint_ctx,
	                                     mint_pub, blk_sig,
	                                     &blk);
	if (rc != MCPKG_RPAGE_NO_ERROR) {
		rc = MCPKG_RPAGE_ERR_CRYPTO;
		goto fail;
	}
	blk->height = block_height;

	/* --- Consistency proof ---
	 * Always return a non-NULL proof object.
	 * If m == 0 (first block) or m == n (no growth), return an empty proof.
	 */
	if (m > 0 && m < n) {
		/* real proof */
		rc = mcpkg_merkle_b2b32_consistency(pg->tree, m, &cons);
		if (rc != MCPKG_MCONS_NO_ERROR) {
			rc = MCPKG_RPAGE_ERR_CRYPTO;
			goto fail;
		}
	} else {
		/* empty proof object with empty nodes list */
		cons = mcpkg_mp_ledger_consistency_new();
		if (!cons) {
			rc = MCPKG_RPAGE_ERR_NO_MEMORY;
			goto fail;
		}
		cons->nodes = mcpkg_list_new(32 /* elem size */, NULL /* POD */, 0, 0);
		if (!cons->nodes) {
			rc = MCPKG_RPAGE_ERR_NO_MEMORY;
			goto fail;
		}
	}

	/* success */
	*out_sth  = sth;
	sth = NULL;
	*out_blk  = blk;
	blk = NULL;
	*out_cons = cons;
	cons = NULL;
	return MCPKG_RPAGE_NO_ERROR;

fail:
	if (cons) mcpkg_mp_ledger_consistency_free(cons);
	if (blk)  mcpkg_mp_ledger_block_free(blk);     /* frees blk->sth */
	else if (sth) mcpkg_mp_ledger_sth_free(sth);
	return rc;
}

MCPKG_API void
mcpkg_repo_page_free(struct McPkgRepoPage *pg)
{
	if (!pg)
		return;
	free(pg);
}
