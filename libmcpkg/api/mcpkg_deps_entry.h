#ifndef MCPKG_DEPS_ENTRY_H
#define MCPKG_DEPS_ENTRY_H

#include <stdint.h>
#include <msgpack.h>
#include <stdbool.h>
#include "utils/array_helper.h"

typedef struct McPkgDeps {
    char *id;              // project_id
    char *name;            // (keep if you want a display name)
    char *version;         // (repurpose or add a new field)
    char *dependency_type; // NEW
    str_array *loaders;
    char *url;
    char *file_name;
    uint64_t size;
} McPkgDeps;

/** Allocate + zero */
McPkgDeps *mcpkg_deps_new(void);

/** Free everything owned by the struct */
void mcpkg_deps_free(McPkgDeps *deps);

/** Pack into an existing msgpack packer (writes a map) */
int mcpkg_deps_pack(msgpack_packer *pk, const McPkgDeps *deps);

/** Unpack from a msgpack object map into an existing struct */
int mcpkg_deps_unpack(msgpack_object *deps_obj, McPkgDeps *deps);

/** Pretty print (caller frees) */
char *mcpkg_deps_to_string(const McPkgDeps *deps);

#endif /* MCPKG_DEPS_ENTRY_H */
