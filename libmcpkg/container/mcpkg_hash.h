#ifndef MCPKG_HASH_H
#define MCPKG_HASH_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "container/mcpkg_container_error.h"

MCPKG_BEGIN_DECLS

/*
 * McPkgHash: string-key → by-value map.
 * - Keys are owned (strdup on insert, free on remove/clear).
 * - Values are stored by value (fixed value_size).
 * - Optional value hooks let you deep-copy / destroy internals.
 * - Unordered iteration; order is not stable.
 * - Not thread-safe; callers must synchronize.
 */

typedef struct McPkgHash McPkgHash;

typedef struct {
	void (*value_copy)(void *dst, const void *src, void *ctx); /* memcpy */
	void (*value_dtor)(void *val, void *ctx);                  /* no-op */
	void *ctx;
} McPkgHashOps;

/* Create a map; caps default or as provided. Returns NULL on failure. */
MCPKG_API McPkgHash *mcpkg_hash_new(size_t value_size,
                                    const McPkgHashOps *ops_or_null,
                                    size_t max_pairs /*0=default*/,
                                    unsigned long long max_bytes /*0=def*/);

/* Free the map, its keys, and call value_dtor for remaining values. */
MCPKG_API void mcpkg_hash_free(McPkgHash *h);

/* Number of entries currently stored. */
MCPKG_API size_t mcpkg_hash_size(const McPkgHash *h);

/* Inspect or update per-instance limits (0 leaves a field unchanged). */
MCPKG_API void mcpkg_hash_get_limits(const McPkgHash *h,
                                     size_t *max_pairs,
                                     unsigned long long *max_bytes);

MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_hash_set_limits(McPkgHash *h, size_t max_pairs,
                      unsigned long long max_bytes);

/* Remove all entries (frees keys; calls value_dtor if provided). */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_hash_remove_all(McPkgHash *h);

/* Remove key if present (frees key; calls value_dtor). */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_hash_remove(McPkgHash *h,
                const char *key);

/* Insert or replace key with *value (dup key if new; copy value by-value). */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_hash_set(McPkgHash *h,
                const char *key,
                const void *value);

/* Copy value for key into *out. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_hash_get(const McPkgHash *h,
                const char *key,
                void *out);

/* If key exists, copy value to out (nullable) then remove the entry. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_hash_pop(McPkgHash *h,
                const char *key,
                void *out /*nullable*/);

/* Return 1 if key exists, 0 if not. */
MCPKG_API int mcpkg_hash_contains(const McPkgHash *h, const char *key);






/*
 * Unordered iteration:
 *   size_t it;
 *   mcpkg_hash_iter_begin(h, &it);
 *   while (mcpkg_hash_iter_next(h, &it, &k, buf_or_null)) { ... }
 */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_hash_iter_begin(const McPkgHash *h,
                size_t *it /*start*/);

MCPKG_API int mcpkg_hash_iter_next(const McPkgHash *h, size_t *it,
                                   const char **key_out,
                                   void *value_out /*nullable*/);

MCPKG_END_DECLS
#endif /* MCPKG_HASH_H */
