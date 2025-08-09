#ifndef MCPKG_ENTRY_H
#define MCPKG_ENTRY_H

#include <stdint.h>
#include <msgpack.h>
#include <cjson/cJSON.h>
#include <stdbool.h>
#include <time.h>

#include "utils/array_helper.h"
#include "api/mcpkg_deps_entry.h"
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS
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

/** Allocates and initializes a new McPkgEntry struct (arrays pre-created). */
McPkgEntry *mcpkg_entry_new(void);

/** Frees all memory associated with an McPkgEntry struct. */
void mcpkg_entry_free(McPkgEntry *entry);

/** Packs an McPkgEntry into a msgpack buffer as a single map. Returns 0 on success. */
int mcpkg_entry_pack(const McPkgEntry *entry, msgpack_sbuffer *sbuf);

/** Unpacks a msgpack object map into an McPkgEntry. Returns 0 on success. */
int mcpkg_entry_unpack(msgpack_object *entry_obj, McPkgEntry *entry);

/** Converts an McPkgEntry to a debug string (caller frees). */
char *mcpkg_entry_to_string(const McPkgEntry *entry);

static void mcpkg_sleep_ms(long milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, &ts);
}
MCPKG_END_DECLS
#endif /* MCPKG_ENTRY_H */
