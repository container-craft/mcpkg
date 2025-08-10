#ifndef MCPKG_INFO_ENTRY_H
#define MCPKG_INFO_ENTRY_H

#include <stdint.h>
#include <msgpack.h>

#include "mcpkg.h"
#include "utils/array_helper.h"
#include "utils/mcpkg_msgpack.h"

#ifdef _WIN32
#include "utils/compat.h"
#endif
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

/*!
 * The main Cache struct
 */
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



McPkgInfoEntry *mcpkg_info_entry_new(void);
void mcpkg_info_entry_free(McPkgInfoEntry *entry);


MCPKG_ERROR_TYPE mcpkg_info_entry_pack(const McPkgInfoEntry *entry,
                                       msgpack_sbuffer *sbuf);
MCPKG_ERROR_TYPE mcpkg_info_entry_unpack(msgpack_object *obj,
                                         McPkgInfoEntry *entry);

char *mcpkg_info_entry_to_string(const McPkgInfoEntry *entry);

MCPKG_END_DECLS
#endif /* MCPKG_INFO_ENTRY_H */
