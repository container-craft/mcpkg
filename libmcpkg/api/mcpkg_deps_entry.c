#include "api/mcpkg_deps_entry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



McPkgDeps *mcpkg_deps_new() {
    McPkgDeps *deps = calloc(1, sizeof(McPkgDeps));
    if (!deps)
        return NULL;
    deps->loaders = str_array_new();
    return deps;
}

void mcpkg_deps_free(McPkgDeps *deps) {
    if (!deps)
        return;
    free(deps->id);
    free(deps->name);
    free(deps->version);
    str_array_free(deps->loaders);
    free(deps->url);
    free(deps->file_name);
    free(deps);
}

int mcpkg_deps_pack(msgpack_packer *pk, const McPkgDeps *deps) {
    msgpack_pack_map(pk, 7); // Number of fields

    msgpack_pack_str_with_body(pk, "id", 2);
    msgpack_pack_str_with_body(pk, deps->id, strlen(deps->id));

    msgpack_pack_str_with_body(pk, "name", 4);
    msgpack_pack_str_with_body(pk, deps->name, strlen(deps->name));

    msgpack_pack_str_with_body(pk, "version", 7);
    msgpack_pack_str_with_body(pk, deps->version, strlen(deps->version));

    msgpack_pack_str_with_body(pk, "loaders", 7);
    str_array_pack(pk, deps->loaders);

    msgpack_pack_str_with_body(pk, "url", 3);
    msgpack_pack_str_with_body(pk, deps->url, strlen(deps->url));

    msgpack_pack_str_with_body(pk, "file_name", 9);
    msgpack_pack_str_with_body(pk, deps->file_name, strlen(deps->file_name));

    msgpack_pack_str_with_body(pk, "size", 4);
    msgpack_pack_uint64(pk, deps->size);

    return 0;
}
int mcpkg_deps_unpack(msgpack_object *deps_obj, McPkgDeps *deps) {
    if (deps_obj->type != MSGPACK_OBJECT_MAP)
        return 1;

    msgpack_object_map map = deps_obj->via.map;
    for (size_t i = 0; i < map.size; ++i) {
        msgpack_object key = map.ptr[i].key;
        msgpack_object val = map.ptr[i].val;

        if (key.type != MSGPACK_OBJECT_STR) continue;

        if (strncmp(key.via.str.ptr, "id", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) deps->id = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "name", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) deps->name = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "version", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) deps->version = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "loaders", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                deps->loaders = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR) {
                        str_array_add(deps->loaders, elem->via.str.ptr);
                    }
                }
            }
        } else if (strncmp(key.via.str.ptr, "url", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) deps->url = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "file_name", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) deps->file_name = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "size", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) deps->size = val.via.u64;
        }
    }
    return 0;
}

char *mcpkg_deps_to_string(const McPkgDeps *deps) {
    if (!deps)
        return strdup("NULL");

    char *output;
    char *loaders_str = str_array_to_string(deps->loaders);

    asprintf(&output,
             "McPkgDeps {\n"
             "  id: %s\n"
             "  name: %s\n"
             "  version: %s\n"
             "  loaders: %s\n"
             "  url: %s\n"
             "  file_name: %s\n"
             "  size: %lu\n"
             "}\n",
             deps->id, deps->name, deps->version, loaders_str, deps->url, deps->file_name, deps->size);

    free(loaders_str);
    return output;
}
