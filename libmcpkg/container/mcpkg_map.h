#ifndef MCPKG_MAP_H
#define MCPKG_MAP_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "container/mcpkg_container_error.h"

MCPKG_BEGIN_DECLS

/*
 * McPkgMap: ordered map (string key → by-value value), ascending strcmp().
 * - Keys are owned (strdup on insert, free on remove/clear).
 * - Values stored by value (fixed value_size).
 * - Optional hooks let you deep-copy / destroy value internals.
 * - !! WARNING funtime police!! Not thread-safe; callers must synchronize.
 */

typedef struct McPkgMap McPkgMap;

typedef struct {
    void (*value_copy)(void *dst, const void *src, void *ctx); /* memcpy */
    void (*value_dtor)(void *val, void *ctx);                  /* no-op */
    void *ctx;
} McPkgMapOps;

/* Create an ordered map; 0 caps = defaults. NULL on failure. */
MCPKG_API McPkgMap *mcpkg_map_new(size_t value_size,
                                  const McPkgMapOps *ops_or_null,
                                  size_t max_pairs,
                                  unsigned long long max_bytes);

/* Free the map, its keys, and call value_dtor for remaining values. */
MCPKG_API void mcpkg_map_free(McPkgMap *m);

/* Number of entries. */
MCPKG_API size_t mcpkg_map_size(const McPkgMap *m);

/* Inspect/update per-instance limits (0 leaves field unchanged). */
MCPKG_API void mcpkg_map_get_limits(const McPkgMap *m,
                                    size_t *max_pairs,
                                    unsigned long long *max_bytes);

MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_map_set_limits(McPkgMap *m, size_t max_pairs,
                     unsigned long long max_bytes);

/* Insert or replace key with *value (dup key if new; copy value). */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_map_set(McPkgMap *m,
                                              const char *key,
                                              const void *value);

/* Copy value for key into *out. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_map_get(const McPkgMap *m,
                                              const char *key,
                                              void *out);

/* Remove key if present (frees key; calls value_dtor). */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_map_remove(McPkgMap *m,
                                                 const char *key);

/* Return 1 if key exists, 0 if not. */
MCPKG_API int mcpkg_map_contains(const McPkgMap *m, const char *key);

/* ---- ordered iteration ----
 * Usage:
 *   void *it;
 *   mcpkg_map_iter_begin(m, &it);
 *   while (mcpkg_map_iter_next(m, &it, &k, buf_or_null)) { ... }
 *   mcpkg_map_iter_seek(m, &it, "k2"); // lower_bound("k2")
 */

MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_map_iter_begin(const McPkgMap *m,
                                                     void **iter);

MCPKG_API int mcpkg_map_iter_next(const McPkgMap *m, void **iter,
                                  const char **key_out,
                                  void *value_out /*nullable*/);

MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_map_iter_seek(const McPkgMap *m,
                                                    void **iter,
                                                    const char *seek_key);

/* Convenience: first/last element (ordered). */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_map_first(const McPkgMap *m,
                                                const char **key_out,
                                                void *value_out /*nullable*/);

MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_map_last(const McPkgMap *m,
                                               const char **key_out,
                                               void *value_out /*nullable*/);

MCPKG_END_DECLS
#endif /* MCPKG_MAP_H */
