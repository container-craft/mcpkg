/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_CRYPTO_MANIFEST_AND_SIG_H
#define MCPKG_CRYPTO_MANIFEST_AND_SIG_H

#include <stdint.h>
#include <stddef.h>

#include "mcpkg_export.h"

/* We reference these MP structs and packers directly */
#include "mp/mcpkg_mp_pkg_meta.h"             /* struct McPkgCache, mcpkg_mp_pkg_meta_pack */
#include "mp/mcpkg_mp_ledger_attestation.h"   /* struct McPkgAttestation */
#include "mp/mcpkg_mp_ledger_sth.h"           /* struct McPkgSTH */
#include "mp/mcpkg_mp_ledger_block.h"         /* struct McPkgBlock */

#include "mcpkg_crypto_hash.h"                /* BLAKE2b-256 */

MCPKG_BEGIN_DECLS

/* error codes for this module */
#define MCPKG_MANIFEST_NO_ERROR   0
#define MCPKG_MANIFEST_ERR_INVALID 1
#define MCPKG_MANIFEST_ERR_NO_MEMORY 2
#define MCPKG_MANIFEST_ERR_PACK   3
#define MCPKG_MANIFEST_ERR_SIGN   4

/* signer callback (used for attestation and block signing) */
typedef int (*mcpkg_signer_cb)(const void *msg, size_t msg_len,
                               uint8_t out_pub32[32], uint8_t out_sig64[64],
                               void *ctx);

/* --- pack meta via mpgen --- */
MCPKG_API int
mcpkg_manifest_pack(const struct McPkgCache *meta,
                    void **out_buf, size_t *out_len);

/* --- BLAKE2b-32(buf) helper --- */
MCPKG_API int
mcpkg_manifest_hash_b2b32(const void *buf, size_t len, uint8_t out32[32]);

/* --- hash already provided, sign and wrap (attestation) --- */
MCPKG_API int
mcpkg_manifest_attest_b2b32(const char *pkg_id,
                            const char *version,
                            const uint8_t manifest_b2b32[32],
                            int64_t ts_ms,
                            mcpkg_signer_cb signer, void *signer_ctx,
                            struct McPkgAttestation **out_att);

/* --- one-shot: pack -> b2b32 -> sign -> att --- */
MCPKG_API int
mcpkg_manifest_attest_from_meta(const struct McPkgCache *meta,
                                int64_t ts_ms,
                                mcpkg_signer_cb signer, void *signer_ctx,
                                struct McPkgAttestation **out_att);

/* --- sign a ledger.block for an STH (prev || sth || fields) --- */
MCPKG_API int
mcpkg_manifest_block_sign_b2b32(const uint8_t prev32[32],
                                const struct McPkgSTH *sth,
                                mcpkg_signer_cb signer, void *signer_ctx,
                                uint8_t out_mint_pub[32],
                                uint8_t out_sig64[64],
                                struct McPkgBlock **out_blk);

MCPKG_END_DECLS

#endif /* MCPKG_CRYPTO_MANIFEST_AND_SIG_H */
