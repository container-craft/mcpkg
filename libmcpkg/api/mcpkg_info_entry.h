#ifndef MCPKG_INFO_ENTRY_H
#define MCPKG_INFO_ENTRY_H

#include <stdint.h>
#include <msgpack.h>
#include "../utils/array_helper.h"

// Main struct for mod/project metadata
typedef struct McPkgInfoEntry {
    char *id;
    char *name;
    char *author;
    char *title;
    char *description;
    char *icon_url;
    str_array *categories;
    str_array *versions;
    uint32_t downloads;
    char *date_modified;
    char *latest_version;
    char *license;
    char *client_side;
    char *server_side;
} McPkgInfoEntry;

/**
 * Allocate and zero a new entry.
 */
McPkgInfoEntry *mcpkg_info_entry_new(void);

/**
 * Free an entry and all its fields.
 */
void mcpkg_info_entry_free(McPkgInfoEntry *entry);

/**
 * Pack an entry into a msgpack buffer.
 * Returns 0 on success.
 */
int mcpkg_info_entry_pack(const McPkgInfoEntry *entry, msgpack_sbuffer *sbuf);

/**
 * Unpack a msgpack object into an entry.
 * Returns 0 on success.
 */
int mcpkg_info_entry_unpack(msgpack_object *obj, McPkgInfoEntry *entry);

/**
 * Convert to human-readable string (caller frees).
 */
char *mcpkg_info_entry_to_string(const McPkgInfoEntry *entry);

#endif /* MCPKG_INFO_ENTRY_H */
