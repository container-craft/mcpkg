#ifndef MCPKG_CACHE_H
#define MCPKG_CACHE_H

#include <msgpack.h>
#include <stdbool.h>
#include <stddef.h>

#include "mcpkg_info_entry.h"

#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

typedef struct McPkgCache McPkgCache;
typedef struct McPkgCache {
    char *base_path;
    McPkgInfoEntry **mods;
    size_t mods_count;
} McPkgCache;

/** Allocates and initializes a new McPkgCache struct. */
McPkgCache *mcpkg_cache_new(void);

/** Frees all memory associated with an McPkgCache struct. */
void mcpkg_cache_free(McPkgCache *cache);

/**
 * Loads the package information from the local cache.
 * @param mod_loader e.g., "fabric"
 * @param version e.g., "1.21.8"
 * @return 0 on success, or a MCPKG_ERROR_TYPE code on failure.
 */
int mcpkg_cache_load(McPkgCache *cache, const char *mod_loader, const char *version);

/**
 * Searches the local cache for packages matching a search term (case-insensitive substring).
 * Caller owns the returned array (but NOT the entries). Use free() on the array.
 */
McPkgInfoEntry **mcpkg_cache_search(McPkgCache *cache, const char *package, size_t *matches_count);

/**
 * Retrieves a single package's information from the cache (exact match on name, case-insensitive).
 * Caller owns the returned C-string (pretty-printed entry) or an empty string if not found.
 */
char *mcpkg_cache_show(McPkgCache *cache, const char *package);

MCPKG_END_DECLS
#endif // MCPKG_CACHE_H
