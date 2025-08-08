#ifndef MCPKG_ENTRY_H
#define MCPKG_ENTRY_H

#include <stdint.h>
#include <msgpack.h>
#include <cjson/cJSON.h>
#include <stdbool.h>

#include "utils/array_helper.h"
#include "api/mcpkg_deps_entry.h"

typedef struct McPkgEntry McPkgEntry;

typedef struct McPkgEntry {
    char *id;
    char *name;
    char *author;
    char *sha;
    str_array *loaders;
    char *url;
    str_array *versions;
    char *version;
    char *file_name;
    char *date_published;
    uint64_t size;
    McPkgDeps **dependencies;
    size_t dependencies_count;
} McPkgEntry;


/**
 * @brief Allocates and initializes a new McPkgEntry struct.
 * @return A pointer to the newly created struct, or NULL on failure.
 */
McPkgEntry *mcpkg_entry_new();

/**
 * @brief Frees all memory associated with an McPkgEntry struct.
 * @param entry The entry to free.
 */
void mcpkg_entry_free(McPkgEntry *entry);

/**
 * @brief Packs an McPkgEntry struct into a msgpack_sbuffer.
 * @param entry The struct to pack.
 * @param sbuf The msgpack buffer to write to.
 * @return 0 on success, non-zero on failure.
 */
int mcpkg_entry_pack(const McPkgEntry *entry, msgpack_sbuffer *sbuf);

/**
 * @brief Unpacks a msgpack object into an McPkgEntry struct.
 * @param entry_obj The msgpack object.
 * @param entry The struct to populate.
 * @return 0 on success, non-zero on failure.
 */
int mcpkg_entry_unpack(msgpack_object *entry_obj, McPkgEntry *entry);

/**
 * @brief Converts an McPkgEntry struct into a debug string.
 * @param entry The entry to convert.
 * @return A dynamically allocated string, or NULL on failure.
 */
char *mcpkg_entry_to_string(const McPkgEntry *entry);

#endif // MCPKG_ENTRY_H
