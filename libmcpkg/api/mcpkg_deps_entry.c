#include "api/mcpkg_deps_entry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>


static int equal_key(msgpack_object k, const char *lit) {
    size_t n = strlen(lit);
    return (k.type == MSGPACK_OBJECT_STR &&
            k.via.str.size == n &&
            memcmp(k.via.str.ptr, lit, n) == 0);
}

static void pack_str_or_nil(msgpack_packer *pk, const char *s) {
    if (!s) { msgpack_pack_nil(pk); return; }
    size_t n = strlen(s);
    msgpack_pack_str(pk, n);
    msgpack_pack_str_body(pk, s, n);
}


McPkgDeps *mcpkg_deps_new(void) {
    McPkgDeps *deps = (McPkgDeps *)calloc(1, sizeof(*deps));
    if (!deps) return NULL;
    deps->loaders = str_array_new();
    return deps;
}

void mcpkg_deps_free(McPkgDeps *deps) {
    if (!deps) return;
    free(deps->id);
    free(deps->name);
    free(deps->version);
    free(deps->dependency_type);
    str_array_free(deps->loaders);
    free(deps->url);
    free(deps->file_name);
    free(deps);
}

int mcpkg_deps_pack(msgpack_packer *pk, const McPkgDeps *deps) {
    if (!pk || !deps) return 1;

    msgpack_pack_map(pk, 7);

    msgpack_pack_str_with_body(pk, "id", 2);
    pack_str_or_nil(pk, deps->id);

    msgpack_pack_str_with_body(pk, "name", 4);
    pack_str_or_nil(pk, deps->name);

    msgpack_pack_str_with_body(pk, "version", 7);
    pack_str_or_nil(pk, deps->version);

    msgpack_pack_str_with_body(pk, "dependency_type", 15);  // NEW
    pack_str_or_nil(pk, deps->dependency_type);

    msgpack_pack_str_with_body(pk, "loaders", 7);
    if (deps->loaders) str_array_pack(pk, deps->loaders); else msgpack_pack_nil(pk);

    msgpack_pack_str_with_body(pk, "url", 3);
    pack_str_or_nil(pk, deps->url);

    msgpack_pack_str_with_body(pk, "file_name", 9);
    pack_str_or_nil(pk, deps->file_name);

    msgpack_pack_str_with_body(pk, "size", 4);
    msgpack_pack_uint64(pk, deps->size);


    return 0;
}

int mcpkg_deps_unpack(msgpack_object *deps_obj, McPkgDeps *deps) {
    if (!deps_obj || deps_obj->type != MSGPACK_OBJECT_MAP || !deps) return 1;

    msgpack_object_map map = deps_obj->via.map;

    for (size_t i = 0; i < map.size; ++i) {
        msgpack_object key = map.ptr[i].key;
        msgpack_object val = map.ptr[i].val;

        if (equal_key(key, "id")) {
            if (val.type == MSGPACK_OBJECT_STR) deps->id = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "name")) {
            if (val.type == MSGPACK_OBJECT_STR) deps->name = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "version")) {
            if (val.type == MSGPACK_OBJECT_STR) deps->version = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "dependency_type")) {
            if (val.type == MSGPACK_OBJECT_STR)
                deps->dependency_type = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "loaders")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                if (!deps->loaders) deps->loaders = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR) {
                        /* Avoid double-alloc: use length-based add */
                        str_array_add_n(deps->loaders, elem->via.str.ptr, elem->via.str.size);
                    }
                }
            }
        } else if (equal_key(key, "url")) {
            if (val.type == MSGPACK_OBJECT_STR) deps->url = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "file_name")) {
            if (val.type == MSGPACK_OBJECT_STR) deps->file_name = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "size")) {
            if (val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) deps->size = val.via.u64;
        }
    }
    return 0;
}

#define S(x) ((x) ? (x) : "(null)")

char *mcpkg_deps_to_string(const McPkgDeps *deps) {
    if (!deps)
        return strdup("NULL");

    char *loaders_str = deps->loaders ? str_array_to_string(deps->loaders) : strdup("[]");

    char *out = NULL;
    /* Use PRIu64 for portable uint64_t printing */
    asprintf(&out,
             "McPkgDeps {\n"
             "  id: %s\n"
             "  name: %s\n"
             "  version: %s\n"
             "  dependency_type: %s\n"
             "  loaders: %s\n"
             "  url: %s\n"
             "  file_name: %s\n"
             "  size: %" PRIu64 "\n"
             "}\n",
             S(deps->id),
             S(deps->name),
             S(deps->version),
             S(deps->dependency_type),
             S(loaders_str),
             S(deps->url),
             S(deps->file_name),
             deps->size);

    free(loaders_str);
    return out;
}
