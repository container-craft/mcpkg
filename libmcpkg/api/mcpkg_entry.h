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

McPkgEntry *mcpkg_entry_new(void);
void mcpkg_entry_free(McPkgEntry *entry);

MCPKG_ERROR_TYPE mcpkg_entry_pack(const McPkgEntry *entry, msgpack_sbuffer *sbuf);
MCPKG_ERROR_TYPE mcpkg_entry_unpack(msgpack_object *entry_obj, McPkgEntry *entry);

// MAKE sure you free what you pass in its new
char *mcpkg_entry_to_string(const McPkgEntry *entry);

MCPKG_END_DECLS
#endif /* MCPKG_ENTRY_H */
