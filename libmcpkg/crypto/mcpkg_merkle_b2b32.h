/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_CRYPTO_MERKLE_B2B32_H
#define MCPKG_CRYPTO_MERKLE_B2B32_H

#include "mcpkg_export.h"
#include <stdint.h>
#include <stddef.h>

struct McPkgSTH;            /* mp: ledger.sth */
struct McPkgAuditPath;      /* mp: ledger.audit_path */

MCPKG_BEGIN_DECLS

/* return codes */
#define MCPKG_MERKLE_NO_ERROR          0
#define MCPKG_MERKLE_ERR_INVALID       1
#define MCPKG_MERKLE_ERR_NO_MEMORY     2
#define MCPKG_MERKLE_ERR_RANGE         3

struct McPkgMerkleB2B32 {
	uint8_t         *leaves;        /* N * 32 */
	size_t          size;           /* count of leaves */
	size_t          cap;            /* capacity in leaves */
};

/* lifecycle */
MCPKG_API struct McPkgMerkleB2B32 *mcpkg_merkle_b2b32_new(size_t cap_hint);
MCPKG_API void mcpkg_merkle_b2b32_free(struct McPkgMerkleB2B32 *t);
MCPKG_API int  mcpkg_merkle_b2b32_reset(struct McPkgMerkleB2B32 *t);

/* append one leaf (digest32). returns 0-based index via out_idx */
MCPKG_API int  mcpkg_merkle_b2b32_append(struct McPkgMerkleB2B32 *t,
                const uint8_t leaf32[32],
                uint64_t *out_idx);

MCPKG_API size_t mcpkg_merkle_b2b32_size(const struct McPkgMerkleB2B32 *t);

/* root hash; dup-last strategy for odd nodes */
MCPKG_API int  mcpkg_merkle_b2b32_root(const struct McPkgMerkleB2B32 *t,
                                       uint8_t out_root32[32]);

/* build STH; last = first + size - 1 */
MCPKG_API int  mcpkg_merkle_b2b32_build_sth(const struct McPkgMerkleB2B32 *t,
                uint64_t first_idx_1based,
                uint64_t ts_ms,
                struct McPkgSTH **out_sth);

/* audit path for leaf idx (0-based). path nodes are ordered bottom-up. */
MCPKG_API int  mcpkg_merkle_b2b32_audit_path(const struct McPkgMerkleB2B32 *t,
                uint64_t leaf_index_0,
                struct McPkgAuditPath **out_path);

MCPKG_END_DECLS
#endif /* MCPKG_CRYPTO_MERKLE_B2B32_H */
