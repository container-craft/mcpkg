/* SPDX-License-Identifier: MIT */
#include "crypto/mcpkg_merkle_consistency_b2b32.h"

#include <stdlib.h>
#include <string.h>

#include "crypto/mcpkg_merkle_b2b32.h"
#include "mcpkg_crypto_hash.h"

#include "mp/mcpkg_mp_ledger_consistency.h"

#include "container/mcpkg_list.h"

#define NODE_SZ 32

/* ---- local level builder (dup-last), mirrors merkle impl ---- */

struct level_buf {
	uint8_t *data;   /* count * 32 */
	size_t   count;  /* nodes in this level */
};

static void free_levels(struct level_buf *lv, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++) free(lv[i].data);
}

static void hpair(const uint8_t *l, const uint8_t *r, uint8_t out[NODE_SZ])
{
	uint8_t buf[NODE_SZ * 2];

	memcpy(buf, l, NODE_SZ);
	memcpy(buf + NODE_SZ, r, NODE_SZ);
	(void)mcpkg_crypto_blake2b32_buf(buf, sizeof(buf), out);
}

static int build_levels_from_tree(const struct McPkgMerkleB2B32 *t,
                                  struct level_buf **out_lv,
                                  size_t *out_lv_count)
{
	struct level_buf *lv = NULL;
	size_t lv_cap = 0, lv_cnt = 0;
	uint8_t *cur = NULL;
	size_t cur_n = 0;

	if (!t || !t->leaves || t->size == 0 || !out_lv || !out_lv_count)
		return MCPKG_MCONS_ERR_INVALID;

	/* level 0 */
	cur = (uint8_t *)malloc(t->size * NODE_SZ);
	if (!cur) return MCPKG_MCONS_ERR_NO_MEMORY;
	memcpy(cur, t->leaves, t->size * NODE_SZ);
	cur_n = t->size;

	lv_cap = 8;
	lv = (struct level_buf *)malloc(lv_cap * sizeof(*lv));
	if (!lv) {
		free(cur);
		return MCPKG_MCONS_ERR_NO_MEMORY;
	}

	lv[0].data = cur;
	lv[0].count = cur_n;
	lv_cnt = 1;

	while (cur_n > 1) {
		size_t next_n = (cur_n + 1) / 2;
		uint8_t *next = (uint8_t *)malloc(next_n * NODE_SZ);
		size_t i, j = 0;

		if (!next) {
			free_levels(lv, lv_cnt);
			free(lv);
			return MCPKG_MCONS_ERR_NO_MEMORY;
		}

		for (i = 0; i + 1 < cur_n; i += 2, j++) {
			const uint8_t *L = cur + (i * NODE_SZ);
			const uint8_t *R = cur + ((i + 1) * NODE_SZ);
			hpair(L, R, next + (j * NODE_SZ));
		}
		if (cur_n & 1) {
			const uint8_t *L = cur + ((cur_n - 1) * NODE_SZ);
			hpair(L, L, next + (j * NODE_SZ));
		}

		if (lv_cnt == lv_cap) {
			size_t nc = lv_cap * 2;
			void *np = realloc(lv, nc * sizeof(*lv));
			if (!np) {
				free(next);
				free_levels(lv, lv_cnt);
				free(lv);
				return MCPKG_MCONS_ERR_NO_MEMORY;
			}
			lv = (struct level_buf *)np;
			lv_cap = nc;
		}
		lv[lv_cnt].data = next;
		lv[lv_cnt].count = next_n;
		lv_cnt++;

		cur = next;
		cur_n = next_n;
	}

	*out_lv = lv;
	*out_lv_count = lv_cnt;
	return MCPKG_MCONS_NO_ERROR;
}

/* ---- helpers for dyadic cover of [m, n) ---- */

static size_t ilog2_zu(size_t x)
{
	size_t k = 0;
	while ((size_t)1 << (k + 1) <= x) k++;
	return k;
}

static size_t pow2_floor(size_t x)
{
	if (x == 0) return 0;
	return (size_t)1 << ilog2_zu(x);
}

/* largest power-of-two that divides x (x>0). if x==0 -> 0 */
static size_t lsb_pow2(size_t x)
{
	if (!x) return 0;
	return x & (~x + 1);
}

/* fetch root for aligned perfect subtree [start, start+size) (size=2^k) */
static int get_aligned_subtree_hash(struct level_buf *lv, size_t lv_cnt,
                                    size_t start, size_t size,
                                    uint8_t out[NODE_SZ])
{
	size_t lvl, idx;

	if (!lv || !lv_cnt || !out || size == 0) return MCPKG_MCONS_ERR_INVALID;

	lvl = ilog2_zu(size);
	if (lvl >= lv_cnt) return MCPKG_MCONS_ERR_RANGE;

	idx = start >> lvl;
	if (idx >= lv[lvl].count) return MCPKG_MCONS_ERR_RANGE;

	memcpy(out, lv[lvl].data + (idx * NODE_SZ), NODE_SZ);
	return MCPKG_MCONS_NO_ERROR;
}

/*
 * Dyadic partition of [m, n): at each step pick the largest aligned block
 * size bs = min( lsbit(m), pow2_floor(n - m) ), where lsbit(0) is treated
 * as "infinite" (so bs = pow2_floor(n - m) when m==0).
 */
static int append_cover_nodes(struct level_buf *lv, size_t lv_cnt,
                              size_t m, size_t n,
                              struct McPkgConsistencyProof *proof)
{
	size_t s = m;

	if (!proof) return MCPKG_MCONS_ERR_INVALID;
	if (n < m) return MCPKG_MCONS_ERR_RANGE;
	if (n == m) return MCPKG_MCONS_NO_ERROR;

	if (!proof->nodes) {
		proof->nodes = mcpkg_list_new(NODE_SZ, NULL, 0, 0);
		if (!proof->nodes) return MCPKG_MCONS_ERR_NO_MEMORY;
	}

	while (s < n) {
		size_t remain = n - s;
		size_t align  = lsb_pow2(s);
		size_t bs;

		if (s == 0) align = pow2_floor(remain);
		if (!align) align = 1;

		bs = pow2_floor(remain);
		if (align < bs) bs = align;

		{
			uint8_t h[NODE_SZ];
			int rc = get_aligned_subtree_hash(lv, lv_cnt, s, bs, h);
			if (rc != MCPKG_MCONS_NO_ERROR) return rc;

			if (mcpkg_list_push(proof->nodes, h) != MCPKG_CONTAINER_OK)
				return MCPKG_MCONS_ERR_NO_MEMORY;
		}

		s += bs;
	}

	return MCPKG_MCONS_NO_ERROR;
}

/* ---- public API ---- */

MCPKG_API int
mcpkg_merkle_b2b32_consistency(const struct McPkgMerkleB2B32 *t,
                               uint64_t old_size_m,
                               struct McPkgConsistencyProof **out_proof)
{
	struct McPkgConsistencyProof *cp = NULL;
	struct level_buf *lv = NULL;
	size_t lv_cnt = 0;
	size_t n, m;
	int rc;

	if (!t || !out_proof) return MCPKG_MCONS_ERR_INVALID;
	if (t->size == 0) return MCPKG_MCONS_ERR_RANGE;

	n = (size_t)t->size;
	if (old_size_m == 0 || old_size_m > (uint64_t)n)
		return MCPKG_MCONS_ERR_RANGE;

	m = (size_t)old_size_m;

	cp = mcpkg_mp_ledger_consistency_new();
	if (!cp) return MCPKG_MCONS_ERR_NO_MEMORY;

	rc = build_levels_from_tree(t, &lv, &lv_cnt);
	if (rc != MCPKG_MCONS_NO_ERROR) {
		mcpkg_mp_ledger_consistency_free(cp);
		return rc;
	}

	/* empty vector is valid when m==n */
	cp->nodes = mcpkg_list_new(NODE_SZ, NULL, 0, 0);
	if (!cp->nodes) {
		free_levels(lv, lv_cnt);
		free(lv);
		mcpkg_mp_ledger_consistency_free(cp);
		return MCPKG_MCONS_ERR_NO_MEMORY;
	}

	if (m < n) {
		rc = append_cover_nodes(lv, lv_cnt, m, n, cp);
		if (rc != MCPKG_MCONS_NO_ERROR) {
			free_levels(lv, lv_cnt);
			free(lv);
			mcpkg_mp_ledger_consistency_free(cp);
			return rc;
		}
	}

	free_levels(lv, lv_cnt);
	free(lv);

	*out_proof = cp;
	return MCPKG_MCONS_NO_ERROR;
}
