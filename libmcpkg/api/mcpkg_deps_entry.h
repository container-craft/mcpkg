#ifndef MCPKG_DEPS_H
#define MCPKG_DEPS_H

#include <stdint.h>
#include <msgpack.h>
#include <cjson/cJSON.h>
#include <stdbool.h>
#include "utils/array_helper.h"

typedef struct McPkgDeps McPkgDeps;
typedef struct McPkgDeps {
    char *id;
    char *name;
    char *version;
    str_array *loaders;
    char *url;
    char *file_name;
    uint64_t size;
} McPkgDeps;

/**
 * @brief Allocates and initializes a new McPkgDeps struct.
 * @return A pointer to the newly created struct, or NULL on failure.
 */
McPkgDeps *mcpkg_deps_new();

/**
 * @brief Frees all memory associated with an McPkgDeps struct.
 * @param deps The deps struct to free.
 */
void mcpkg_deps_free(McPkgDeps *deps);

/**
 * @brief Packs an McPkgDeps struct into a msgpack_sbuffer.
 * @param deps The struct to pack.
 * @param sbuf The msgpack buffer to write to.
 * @return 0 on success, non-zero on failure.
 */
int mcpkg_deps_pack(msgpack_packer *pk, const McPkgDeps *deps);

/**
 * @brief Unpacks a msgpack object into an McPkgDeps struct.
 * @param deps_obj The msgpack object.
 * @param deps The struct to populate.
 * @return 0 on success, non-zero on failure.
 */
int mcpkg_deps_unpack(msgpack_object *deps_obj, McPkgDeps *deps);

/**
 * @brief Converts an McPkgDeps struct into a debug string.
 * @param deps The entry to convert.
 * @return A dynamically allocated string, or NULL on failure.
 */
char *mcpkg_deps_to_string(const McPkgDeps *deps);

#endif // MCPKG_DEPS_H
