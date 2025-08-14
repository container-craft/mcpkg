/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_CRYPTO_REPO_PAGE_H
#define MCPKG_CRYPTO_REPO_PAGE_H

#include "mcpkg_export.h"

#include <stdint.h>
#include <stddef.h>

struct McPkgMerkleB2B32;		/* crypto/mcpkg_merkle_b2b32.h */
struct McPkgLeafIndexCache;		/* crypto/mcpkg_leaf_index_cache.h */

struct McPkgCache;			/* mp: pkg.meta */
struct McPkgAttestation;		/* mp: ledger.attestation */
struct McPkgSTH;			/* mp: ledger.sth */
struct McPkgBlock;			/* mp: ledger.block */
struct McPkgConsistencyProof;		/* mp: ledger.consistency */

/* return codes */
#define MCPKG_RPAGE_NO_ERROR            0
#define MCPKG_RPAGE_ERR_INVALID         1
#define MCPKG_RPAGE_ERR_NO_MEMORY       2
#define MCPKG_RPAGE_ERR_CRYPTO          3
#define MCPKG_RPAGE_ERR_MP              4
#define MCPKG_RPAGE_ERR_STATE           5
#define MCPKG_RPAGE_ERR_RANGE           6
/* signer callback: fill pub32_out and sig64_out; return 0 on success */
typedef int (*mcpkg_signer_cb)(const void *msg, size_t msg_len,
                               uint8_t sig64_out[64],
                               uint8_t pub32_out[32],
                               void *ctx);

struct McPkgRepoPage;

/* begin a page: tree+lic are borrowed; provider is not copied */
MCPKG_API int
mcpkg_repo_page_begin(struct McPkgMerkleB2B32 *tree,
                      struct McPkgLeafIndexCache *lic,
                      const char *provider,
                      int64_t ts_ms,
                      mcpkg_signer_cb att_signer, void *att_ctx,
                      mcpkg_signer_cb mint_signer, void *mint_ctx,
                      struct McPkgRepoPage **out_pg);

/* add one package meta: hashes, appends to tree, signs attestation */
MCPKG_API int
mcpkg_repo_page_add(struct McPkgRepoPage *pg,
                    const struct McPkgCache *meta,
                    struct McPkgAttestation **out_att,
                    uint64_t *out_leaf_index);

/* finish page: builds STH, block (signed), and optional consistency proof */
MCPKG_API int
mcpkg_repo_page_finish(struct McPkgRepoPage *pg,
                       const uint8_t prev32[32],
                       uint64_t height,
                       struct McPkgSTH **out_sth,
                       struct McPkgBlock **out_blk,
                       struct McPkgConsistencyProof **out_cons);

/* free page object (does not free tree or lic) */
MCPKG_API void
mcpkg_repo_page_free(struct McPkgRepoPage *pg);

#endif /* MCPKG_CRYPTO_REPO_PAGE_H */
