/* SPDX-License-Identifier: MIT */
#include "mcpkg_leaf_index_cache.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "container/mcpkg_list.h"

struct entry {
	char		*key;		/* provider:project_id[:version_id] */
	uint64_t	index0;		/* 0-based global leaf index */
};

struct McPkgLeafIndexCache {
	struct McPkgList *ents;		/* array<struct entry> */
};

static char *dup_cstr(const char *s)
{
	size_t n;
	char *d;

	if (!s) return NULL;
	n = strlen(s) + 1U;
	d = (char *)malloc(n);
	if (!d) return NULL;
	memcpy(d, s, n);
	return d;
}

/* mcpkg_list dtor signature is (void *ctx, void *elt) */
static void entry_dtor_ctx(void *ctx, void *elt)
{
	(void)ctx;
	struct entry *e = (struct entry *)elt;
	if (!e) return;
	if (e->key) {
		free(e->key);
		e->key = NULL;
	}
}

static const McPkgListOps ENTRY_OPS = {
	.dtor = entry_dtor_ctx,
};

static int valid_part(const char *s)
{
	return s && *s && strchr(s, ':') == NULL;
}

MCPKG_API int
mcpkg_leaf_index_key_from_origin(const char *provider,
                                 const char *project_id,
                                 const char *version_id_or_null,
                                 char **out_key)
{
	size_t need;
	char *k;

	if (!out_key) return MCPKG_LIC_ERR_INVALID;
	*out_key = NULL;

	if (!valid_part(provider) || !valid_part(project_id))
		return MCPKG_LIC_ERR_INVALID;

	/* "prov:proj" or "prov:proj:ver" */
	need = strlen(provider) + 1 + strlen(project_id) + 1 +
	       (version_id_or_null
	        && *version_id_or_null ? strlen(version_id_or_null) + 1 : 0);
	k = (char *)malloc(need);
	if (!k) return MCPKG_LIC_ERR_NO_MEMORY;

	if (version_id_or_null && *version_id_or_null)
		snprintf(k, need, "%s:%s:%s", provider, project_id, version_id_or_null);
	else
		snprintf(k, need, "%s:%s", provider, project_id);

	*out_key = k;
	return MCPKG_LIC_NO_ERROR;
}

MCPKG_API struct McPkgLeafIndexCache *
mcpkg_leaf_index_cache_new(void)
{
	struct McPkgLeafIndexCache *c;

	c = (struct McPkgLeafIndexCache *)calloc(1, sizeof(*c));
	if (!c) return NULL;

	c->ents = mcpkg_list_new(sizeof(struct entry), &ENTRY_OPS, 0, 0);
	if (!c->ents) {
		free(c);
		return NULL;
	}
	return c;
}

MCPKG_API void
mcpkg_leaf_index_cache_free(struct McPkgLeafIndexCache *c)
{
	if (!c) return;
	if (c->ents) mcpkg_list_free(c->ents);
	free(c);
}

static int find_key(struct McPkgLeafIndexCache *c, const char *key,
                    size_t *out_idx)
{
	size_t i, n;

	if (out_idx) *out_idx = (size_t) -1;
	if (!c || !c->ents || !key) return 0;

	n = mcpkg_list_size(c->ents);
	for (i = 0; i < n; i++) {
		struct entry *e = (struct entry *)mcpkg_list_at_ptr(c->ents, i);
		if (e && e->key && strcmp(e->key, key) == 0) {
			if (out_idx) *out_idx = i;
			return 1;
		}
	}
	return 0;
}

MCPKG_API int
mcpkg_leaf_index_cache_set(struct McPkgLeafIndexCache *c,
                           const char *key, uint64_t index0)
{
	struct entry e;
	size_t idx;

	if (!c || !key) return MCPKG_LIC_ERR_INVALID;

	if (find_key(c, key, &idx)) {
		struct entry *cur = (struct entry *)mcpkg_list_at_ptr(c->ents, idx);
		if (!cur) return MCPKG_LIC_ERR_INTERNAL;
		cur->index0 = index0;
		return MCPKG_LIC_NO_ERROR;
	}

	memset(&e, 0, sizeof(e));
	e.key = dup_cstr(key);
	e.index0 = index0;
	if (!e.key) return MCPKG_LIC_ERR_NO_MEMORY;

	if (mcpkg_list_push(c->ents, &e) != MCPKG_CONTAINER_OK) {
		free(e.key);
		return MCPKG_LIC_ERR_NO_MEMORY;
	}
	return MCPKG_LIC_NO_ERROR;
}

MCPKG_API int
mcpkg_leaf_index_cache_get(struct McPkgLeafIndexCache *c,
                           const char *key, uint64_t *out_index0)
{
	size_t idx;

	if (!c || !key || !out_index0) return MCPKG_LIC_ERR_INVALID;

	if (!find_key(c, key, &idx))
		return MCPKG_LIC_ERR_NOT_FOUND;

	{
		struct entry *e = (struct entry *)mcpkg_list_at_ptr(c->ents, idx);
		if (!e) return MCPKG_LIC_ERR_INTERNAL;
		*out_index0 = e->index0;
		return MCPKG_LIC_NO_ERROR;
	}
}
