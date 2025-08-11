#ifndef MCPKG_ENTRY_H
#define MCPKG_ENTRY_H

#include <stdarg.h>
#include <stdint.h>
#include <msgpack.h>
#include <cjson/cJSON.h>
#include <time.h>

#include "mcpkg.h"
#include "utils/array_helper.h"


#include "api/mcpkg_deps_entry.h"
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

/*!
 * The main INSTALL struct
 */


    char *id;              // project_id
    char *name;            // (keep if you want a display name)
    char *version;         // (repurpose or add a new field)
    char *dependency_type; // NEW
    str_array *loaders;
    char *url;
    char *file_name;
    uint64_t size;


typedef struct McPkCache{
    char *id;
    char *slug;
    char *version;
    char *title;
    char *description;
    char *icon_url;
    char *author;
    char *license;
    char *client_side;
    char *server_side;
}McPkCache;

typedef struct McPkgEntry {
    char *id;
    char *slug;
    char *url;
    uint64_t file_size;
    char *file_name;
    char *file_sha;
    McPkCache *cache;
    McpStrList *categories;
    McpStrList *loaders;
    McpStrList *versions;

    McpStructList<> struct

    McpStrList *dependencies;
    size_t dependencies_count;
} McPkgEntry;

McPkgEntry *mcpkg_entry_new(void);
void mcpkg_entry_free(McPkgEntry *entry);

MCPKG_ERROR_TYPE mcpkg_entry_pack(const McPkgEntry *entry, msgpack_sbuffer *sbuf);
MCPKG_ERROR_TYPE mcpkg_entry_unpack(msgpack_object *entry_obj, McPkgEntry *entry);

// MAKE sure you free what you pass in its new
char *mcpkg_entry_to_string(const McPkgEntry *entry);

MCPKG_END_DECLS
#endif /* MCPKG_ENTRY_H */
