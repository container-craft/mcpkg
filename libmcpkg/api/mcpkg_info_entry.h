#ifndef MCPKG_INFO_ENTRY_H
#define MCPKG_INFO_ENTRY_H

#include <stdint.h>
#include <msgpack.h>
#include <cjson/cJSON.h>
#include <stdbool.h>
#include "utils/array_helper.h"

typedef struct {
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
 * @brief Allocates and initializes an McPkgInfoEntry struct.
 * @return A pointer to the newly created struct, or NULL on failure.
 */
McPkgInfoEntry *mcpkg_info_entry_new();

/**
 * @brief Frees all memory associated with an McPkgInfoEntry struct.
 * @param entry The entry to free.
 */
void mcpkg_info_entry_free(McPkgInfoEntry *entry);

/**
 * @brief Packs an McPkgInfoEntry struct into a msgpack_sbuffer.
 * @param entry The struct to pack.
 * @param sbuf The msgpack buffer to write to.
 * @return 0 on success, non-zero on failure.
 */
int mcpkg_info_entry_pack(const McPkgInfoEntry *entry, msgpack_sbuffer *sbuf);

/**
 * @brief Unpacks a msgpack object into an McPkgInfoEntry struct.
 * @param entry_obj The msgpack object.
 * @param entry The struct to populate.
 * @return 0 on success, non-zero on failure.
 */
int mcpkg_info_entry_unpack(msgpack_object *entry_obj, McPkgInfoEntry *entry);

/**
 * @brief Converts an McPkgInfoEntry struct into a debug string.
 * @param entry The entry to convert.
 * @return A dynamically allocated string, or NULL on failure.
 */
char *mcpkg_info_entry_to_string(const McPkgInfoEntry *entry);

#endif // MCPKG_INFO_ENTRY_H
