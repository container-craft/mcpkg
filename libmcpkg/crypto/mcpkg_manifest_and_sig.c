/* SPDX-License-Identifier: MIT */
#include "crypto/mcpkg_manifest_and_sig.h"

#include <stdlib.h>
#include <string.h>

/* ---------------- small helpers ---------------- */

static void pack_u64_le(uint8_t out[8], uint64_t v)
{
	out[0] = (uint8_t)(v);
	out[1] = (uint8_t)(v >>  8);
	out[2] = (uint8_t)(v >> 16);
	out[3] = (uint8_t)(v >> 24);
	out[4] = (uint8_t)(v >> 32);
	out[5] = (uint8_t)(v >> 40);
	out[6] = (uint8_t)(v >> 48);
	out[7] = (uint8_t)(v >> 56);
}

/* ---------------- pack & hash ---------------- */

MCPKG_API int
mcpkg_manifest_pack(const struct McPkgCache *meta,
                    void **out_buf, size_t *out_len)
{
	int mpret;

	if (!meta || !out_buf || !out_len)
		return MCPKG_MANIFEST_ERR_INVALID;

	mpret = mcpkg_mp_pkg_meta_pack(meta, out_buf, out_len);
	if (mpret != 0)
		return MCPKG_MANIFEST_ERR_PACK;

	return MCPKG_MANIFEST_NO_ERROR;
}

MCPKG_API int
mcpkg_manifest_hash_b2b32(const void *buf, size_t len, uint8_t out32[32])
{
	if (!buf || !out32)
		return MCPKG_MANIFEST_ERR_INVALID;
	if (mcpkg_crypto_blake2b32_buf(buf, len, out32) != 0)
		return MCPKG_MANIFEST_ERR_PACK; /* reuse */
	return MCPKG_MANIFEST_NO_ERROR;
}

/* ---------------- attestation ---------------- */

static int fill_attestation(const char *pkg_id,
                            const char *version,
                            const uint8_t manifest_b2b32[32],
                            const uint8_t pub[32],
                            const uint8_t sig[64],
                            int64_t ts_ms,
                            struct McPkgAttestation **out_att)
{
	struct McPkgAttestation *att;

	if (!pkg_id || !pkg_id[0] || !version || !version[0] ||
	    !manifest_b2b32 || !pub || !sig || !out_att)
		return MCPKG_MANIFEST_ERR_INVALID;

	att = mcpkg_mp_ledger_attestation_new();
	if (!att)
		return MCPKG_MANIFEST_ERR_NO_MEMORY;

	att->pkg_id  = strdup(pkg_id);
	att->version = strdup(version);
	if (!att->pkg_id || !att->version) {
		mcpkg_mp_ledger_attestation_free(att);
		return MCPKG_MANIFEST_ERR_NO_MEMORY;
	}

	/* schema field is named manifest_sha256, but we store BLAKE2b-256 */
	memcpy(att->manifest_sha256, manifest_b2b32, 32);
	memcpy(att->signer_pub,      pub,            32);
	memcpy(att->signature,       sig,            64);
	att->ts_ms = ts_ms;

	*out_att = att;
	return MCPKG_MANIFEST_NO_ERROR;
}

MCPKG_API int
mcpkg_manifest_attest_b2b32(const char *pkg_id,
                            const char *version,
                            const uint8_t manifest_b2b32[32],
                            int64_t ts_ms,
                            mcpkg_signer_cb signer, void *signer_ctx,
                            struct McPkgAttestation **out_att)
{
	uint8_t pub[32], sig[64];
	int rc;

	if (!pkg_id || !version || !manifest_b2b32 || !signer || !out_att)
		return MCPKG_MANIFEST_ERR_INVALID;

	/* Sign the leaf hash directly (32 bytes). */
	rc = signer(manifest_b2b32, 32, pub, sig, signer_ctx);
	if (rc != 0)
		return MCPKG_MANIFEST_ERR_SIGN;

	return fill_attestation(pkg_id, version, manifest_b2b32, pub, sig, ts_ms,
	                        out_att);
}

MCPKG_API int
mcpkg_manifest_attest_from_meta(const struct McPkgCache *meta,
                                int64_t ts_ms,
                                mcpkg_signer_cb signer, void *signer_ctx,
                                struct McPkgAttestation **out_att)
{
	void *buf = NULL;
	size_t len = 0;
	uint8_t h[32];
	int rc;

	if (!meta || !signer || !out_att)
		return MCPKG_MANIFEST_ERR_INVALID;

	rc = mcpkg_manifest_pack(meta, &buf, &len);
	if (rc != MCPKG_MANIFEST_NO_ERROR)
		return rc;

	rc = mcpkg_manifest_hash_b2b32(buf, len, h);
	if (rc != MCPKG_MANIFEST_NO_ERROR) {
		free(buf);
		return rc;
	}

	rc = mcpkg_manifest_attest_b2b32(meta->id, meta->version, h, ts_ms,
	                                 signer, signer_ctx, out_att);

	free(buf);
	return rc;
}

/* ---------------- block signer ---------------- */

MCPKG_API int
mcpkg_manifest_block_sign_b2b32(const uint8_t prev32[32],
                                const struct McPkgSTH *sth,
                                mcpkg_signer_cb signer, void *signer_ctx,
                                uint8_t out_mint_pub[32],
                                uint8_t out_sig64[64],
                                struct McPkgBlock **out_blk)
{
	struct McPkgBlock *blk = NULL;
	struct McPkgSTH *sth_copy = NULL;
	uint8_t msg[32 + 32 + 8 + 8 + 8 +
	               8]; /* prev || root || size || ts_ms || first || last */
	size_t off = 0;
	int rc;

	if (!prev32 || !sth || !signer || !out_mint_pub || !out_sig64 || !out_blk)
		return MCPKG_MANIFEST_ERR_INVALID;

	*out_blk = NULL;

	memcpy(msg + off, prev32,    32);
	off += 32;
	memcpy(msg + off, sth->root, 32);
	off += 32;
	pack_u64_le(msg + off, sth->size);
	off += 8;
	pack_u64_le(msg + off, sth->ts_ms);
	off += 8;
	pack_u64_le(msg + off, sth->first);
	off += 8;
	pack_u64_le(msg + off, sth->last);
	off += 8;

	rc = signer(msg, off, out_mint_pub, out_sig64, signer_ctx);
	if (rc != 0)
		return MCPKG_MANIFEST_ERR_SIGN;

	blk = mcpkg_mp_ledger_block_new();
	if (!blk) return MCPKG_MANIFEST_ERR_NO_MEMORY;

	sth_copy = mcpkg_mp_ledger_sth_new();
	if (!sth_copy) {
		mcpkg_mp_ledger_block_free(blk);
		return MCPKG_MANIFEST_ERR_NO_MEMORY;
	}

	memcpy(blk->prev,     prev32,       32);
	memcpy(blk->mint_pub, out_mint_pub, 32);
	memcpy(blk->sig,      out_sig64,    64);

	sth_copy->size  = sth->size;
	memcpy(sth_copy->root, sth->root, 32);
	sth_copy->ts_ms = sth->ts_ms;
	sth_copy->first = sth->first;
	sth_copy->last  = sth->last;

	blk->sth = sth_copy;

	*out_blk = blk;
	return MCPKG_MANIFEST_NO_ERROR;
}
