/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_LEAF_INDEX_CACHE_H
#define MCPKG_LEAF_INDEX_CACHE_H

#include "mcpkg_export.h"
#include <stdint.h>
#include <stddef.h>

MCPKG_BEGIN_DECLS

/* return codes */
#define MCPKG_LIC_NO_ERROR        0   /* success */
#define MCPKG_LIC_ERR_INVALID     1   /* bad args */
#define MCPKG_LIC_ERR_NO_MEMORY   2   /* alloc failed */
#define MCPKG_LIC_ERR_NOT_FOUND   3   /* key not present */
#define MCPKG_LIC_ERR_INTERNAL    4   /* invariant failed / unexpected state */

struct McPkgLeafIndexCache;

/* build "provider:project_id[:version_id]" into *out_key (malloc'd) */
MCPKG_API int
mcpkg_leaf_index_key_from_origin(const char *provider,
                                 const char *project_id,
                                 const char *version_id_or_null,
                                 char **out_key);

/* lifecycle */
MCPKG_API struct McPkgLeafIndexCache *
mcpkg_leaf_index_cache_new(void);

MCPKG_API void
mcpkg_leaf_index_cache_free(struct McPkgLeafIndexCache *c);

/* set/get */
MCPKG_API int
mcpkg_leaf_index_cache_set(struct McPkgLeafIndexCache *c,
                           const char *key, uint64_t index0);

MCPKG_API int
mcpkg_leaf_index_cache_get(struct McPkgLeafIndexCache *c,
                           const char *key, uint64_t *out_index0);

MCPKG_END_DECLS
#endif /* MCPKG_LEAF_INDEX_CACHE_H */
