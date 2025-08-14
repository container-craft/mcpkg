/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_CRYPTO_MERKLE_CONSISTENCY_B2B32_H
#define MCPKG_CRYPTO_MERKLE_CONSISTENCY_B2B32_H

#include "mcpkg_export.h"
#include <stdint.h>
#include <stddef.h>

struct McPkgMerkleB2B32;        /* from crypto/mcpkg_merkle_b2b32.h */
struct McPkgConsistencyProof;   /* mp: ledger.consistency */

MCPKG_BEGIN_DECLS

/* return codes */
#define MCPKG_MCONS_NO_ERROR         0
#define MCPKG_MCONS_ERR_INVALID      1
#define MCPKG_MCONS_ERR_NO_MEMORY    2
#define MCPKG_MCONS_ERR_RANGE        3

/*
* Build a consistency proof between size=m and size=n (n = current tree size).
* Tree uses dup-last at each level (same as crypto/mcpkg_merkle_b2b32.c).
* Nodes are the canonical dyadic cover of [m, n) in left-to-right order,
* each as a 32-byte subtree hash.
*/
MCPKG_API int
mcpkg_merkle_b2b32_consistency(const struct McPkgMerkleB2B32 *t,
                               uint64_t old_size_m,
                               struct McPkgConsistencyProof **out_proof);

MCPKG_END_DECLS
#endif /* MCPKG_CRYPTO_MERKLE_CONSISTENCY_B2B32_H */
