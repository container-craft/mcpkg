/* SPDX-License-Identifier: MIT */
#include "crypto/mcpkg_merkle_b2b32.h"

#include <stdlib.h>
#include <string.h>

#include "mcpkg_crypto_hash.h"

#include "mp/mcpkg_mp_ledger_sth.h"
#include "mp/mcpkg_mp_ledger_audit_path.h"
#include "mp/mcpkg_mp_ledger_audit_node.h"

#define NODE_SZ 32

static int grow(struct McPkgMerkleB2B32 *t, size_t need_cap)
{
	uint8_t *p;
	size_t newcap;

	if (!t) return MCPKG_MERKLE_ERR_INVALID;
	if (t->cap >= need_cap) return MCPKG_MERKLE_NO_ERROR;

	newcap = t->cap ? t->cap : 8;
	while (newcap < need_cap) newcap *= 2;

	p = (uint8_t *)realloc(t->leaves, newcap * NODE_SZ);
	if (!p) return MCPKG_MERKLE_ERR_NO_MEMORY;

	t->leaves = p;
	t->cap = newcap;
	return MCPKG_MERKLE_NO_ERROR;
}

static void hpair(const uint8_t *l, const uint8_t *r, uint8_t out[NODE_SZ])
{
	uint8_t buf[NODE_SZ * 2];

	memcpy(buf, l, NODE_SZ);
	memcpy(buf + NODE_SZ, r, NODE_SZ);
	(void)mcpkg_crypto_blake2b32_buf(buf, sizeof(buf), out);
}

/* build full level pyramid (level[0]=leaves) for path/root */
struct level_buf {
	uint8_t *data;   /* count * 32 */
	size_t   count;  /* nodes in this level */
};

static void free_levels(struct level_buf *lv, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++) free(lv[i].data);
}

static int build_levels(const uint8_t *leaves, size_t n_leaves,
                        struct level_buf **out_lv, size_t *out_lv_count)
{
	struct level_buf *lv = NULL;
	size_t lv_cap = 0, lv_cnt = 0;
	uint8_t *cur = NULL;
	size_t cur_n = 0;

	if (!leaves || n_leaves == 0 || !out_lv || !out_lv_count)
		return MCPKG_MERKLE_ERR_INVALID;

	/* level 0 */
	cur = (uint8_t *)malloc(n_leaves * NODE_SZ);
	if (!cur) return MCPKG_MERKLE_ERR_NO_MEMORY;
	memcpy(cur, leaves, n_leaves * NODE_SZ);
	cur_n = n_leaves;

	lv_cap = 8;
	lv = (struct level_buf *)malloc(lv_cap * sizeof(*lv));
	if (!lv) {
		free(cur);
		return MCPKG_MERKLE_ERR_NO_MEMORY;
	}

	/* push level 0 */
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
			return MCPKG_MERKLE_ERR_NO_MEMORY;
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
				return MCPKG_MERKLE_ERR_NO_MEMORY;
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
	return MCPKG_MERKLE_NO_ERROR;
}

/* ---- public API ---- */

MCPKG_API struct McPkgMerkleB2B32 *
mcpkg_merkle_b2b32_new(size_t cap_hint)
{
	struct McPkgMerkleB2B32 *t;

	t = (struct McPkgMerkleB2B32 *)calloc(1, sizeof(*t));
	if (!t) return NULL;
	if (cap_hint) {
		t->leaves = (uint8_t *)malloc(cap_hint * NODE_SZ);
		if (!t->leaves) {
			free(t);
			return NULL;
		}
		t->cap = cap_hint;
	}
	return t;
}

MCPKG_API void
mcpkg_merkle_b2b32_free(struct McPkgMerkleB2B32 *t)
{
	if (!t) return;
	free(t->leaves);
	free(t);
}

MCPKG_API int
mcpkg_merkle_b2b32_reset(struct McPkgMerkleB2B32 *t)
{
	if (!t) return MCPKG_MERKLE_ERR_INVALID;
	t->size = 0;
	return MCPKG_MERKLE_NO_ERROR;
}

MCPKG_API int
mcpkg_merkle_b2b32_append(struct McPkgMerkleB2B32 *t,
                          const uint8_t leaf32[32],
                          uint64_t *out_idx)
{
	int rc;

	if (!t || !leaf32) return MCPKG_MERKLE_ERR_INVALID;

	rc = grow(t, t->size + 1);
	if (rc != MCPKG_MERKLE_NO_ERROR) return rc;

	memcpy(t->leaves + (t->size * NODE_SZ), leaf32, NODE_SZ);
	if (out_idx) *out_idx = (uint64_t)t->size;
	t->size++;
	return MCPKG_MERKLE_NO_ERROR;
}

MCPKG_API size_t
mcpkg_merkle_b2b32_size(const struct McPkgMerkleB2B32 *t)
{
	return t ? t->size : 0;
}

MCPKG_API int
mcpkg_merkle_b2b32_root(const struct McPkgMerkleB2B32 *t,
                        uint8_t out_root32[32])
{
	struct level_buf *lv = NULL;
	size_t lv_cnt = 0;
	int rc;

	if (!t || !out_root32 || t->size == 0)
		return MCPKG_MERKLE_ERR_INVALID;

	rc = build_levels(t->leaves, t->size, &lv, &lv_cnt);
	if (rc != MCPKG_MERKLE_NO_ERROR) return rc;

	memcpy(out_root32, lv[lv_cnt - 1].data, NODE_SZ);
	free_levels(lv, lv_cnt);
	free(lv);
	return MCPKG_MERKLE_NO_ERROR;
}

MCPKG_API int
mcpkg_merkle_b2b32_build_sth(const struct McPkgMerkleB2B32 *t,
                             uint64_t first_idx_1based,
                             uint64_t ts_ms,
                             struct McPkgSTH **out_sth)
{
	uint8_t root[NODE_SZ];
	struct McPkgSTH *sth;
	int rc;

	if (!t || !out_sth || t->size == 0)
		return MCPKG_MERKLE_ERR_INVALID;

	rc = mcpkg_merkle_b2b32_root(t, root);
	if (rc != MCPKG_MERKLE_NO_ERROR) return rc;

	sth = mcpkg_mp_ledger_sth_new();
	if (!sth) return MCPKG_MERKLE_ERR_NO_MEMORY;

	sth->size = (uint64_t)t->size;
	memcpy(sth->root, root, NODE_SZ);
	sth->ts_ms = ts_ms;
	sth->first = first_idx_1based ? first_idx_1based : 1u;
	sth->last  = sth->first + sth->size - 1u;

	*out_sth = sth;
	return MCPKG_MERKLE_NO_ERROR;
}

MCPKG_API int
mcpkg_merkle_b2b32_audit_path(const struct McPkgMerkleB2B32 *t,
                              uint64_t leaf_index_0,
                              struct McPkgAuditPath **out_path)
{
	struct level_buf *lv = NULL;
	size_t lv_cnt = 0, level = 0;
	uint64_t idx = leaf_index_0;
	struct McPkgAuditPath *ap = NULL;
	int rc;

	if (!t || !out_path) return MCPKG_MERKLE_ERR_INVALID;
	if (t->size == 0) return MCPKG_MERKLE_ERR_RANGE;
	if (idx >= (uint64_t)t->size) return MCPKG_MERKLE_ERR_RANGE;

	rc = build_levels(t->leaves, t->size, &lv, &lv_cnt);
	if (rc != MCPKG_MERKLE_NO_ERROR) return rc;

	ap = mcpkg_mp_ledger_audit_path_new();
	if (!ap) {
		free_levels(lv, lv_cnt);
		free(lv);
		return MCPKG_MERKLE_ERR_NO_MEMORY;
	}

	ap->nodes = mcpkg_list_new(sizeof(struct McPkgAuditNode *), NULL, 0, 0);
	if (!ap->nodes) {
		mcpkg_mp_ledger_audit_path_free(ap);
		free_levels(lv, lv_cnt);
		free(lv);
		return MCPKG_MERKLE_ERR_NO_MEMORY;
	}

	for (level = 0; level + 1 < lv_cnt; level++) {
		size_t n = lv[level].count;
		size_t i = (size_t)idx;
		size_t sib_i;
		int is_right;

		if ((i % 2) == 0) {
			sib_i = (i + 1 < n) ? (i + 1) : i; /* dup-last */
			is_right = 1; /* sibling at right in hash(L||R) */
		} else {
			sib_i = i - 1;
			is_right = 0; /* sibling at left */
		}

		{
			struct McPkgAuditNode *an = mcpkg_mp_ledger_audit_node_new();
			if (!an) {
				mcpkg_mp_ledger_audit_path_free(ap);
				free_levels(lv, lv_cnt);
				free(lv);
				return MCPKG_MERKLE_ERR_NO_MEMORY;
			}
			memcpy(an->sibling, lv[level].data + (sib_i * NODE_SZ), NODE_SZ);
			an->is_right = (uint32_t)is_right;

			if (mcpkg_list_push(ap->nodes, &an) != MCPKG_CONTAINER_OK) {
				mcpkg_mp_ledger_audit_node_free(an);
				mcpkg_mp_ledger_audit_path_free(ap);
				free_levels(lv, lv_cnt);
				free(lv);
				return MCPKG_MERKLE_ERR_NO_MEMORY;
			}
		}

		idx = idx / 2;
	}

	free_levels(lv, lv_cnt);
	free(lv);

	*out_path = ap;
	return MCPKG_MERKLE_NO_ERROR;
}
