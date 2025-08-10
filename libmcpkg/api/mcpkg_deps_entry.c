#include "api/mcpkg_deps_entry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "mcpkg.h"
#include "utils/mcpkg_msgpack.h"

#ifdef _WIN32
    #include "utils/compat.h"
#endif

McPkgDeps *mcpkg_deps_new(void)
{
    McPkgDeps *deps = (McPkgDeps *)calloc(1, sizeof(*deps));
    if (!deps)
        return NULL;

    deps->loaders = str_array_new();
    return deps;
}

//TODO double check 4am code
void mcpkg_deps_free(McPkgDeps *deps)
{
    if (!deps)
        return;

    free(deps->id);
    free(deps->name);
    free(deps->version);
    free(deps->dependency_type);
    str_array_free(deps->loaders);
    free(deps->url);
    free(deps->file_name);
    free(deps);
}

MCPKG_ERROR_TYPE mcpkg_deps_pack(msgpack_packer *pk,
                                 const McPkgDeps *deps)
{
    if (!pk || !deps)
        return MCPKG_ERROR_OOM;

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
    if (deps->loaders)
        str_array_pack(pk, deps->loaders);
    else
        msgpack_pack_nil(pk);

    msgpack_pack_str_with_body(pk, "url", 3);
    pack_str_or_nil(pk, deps->url);

    msgpack_pack_str_with_body(pk, "file_name", 9);
    pack_str_or_nil(pk, deps->file_name);

    msgpack_pack_str_with_body(pk, "size", 4);
    msgpack_pack_uint64(pk, deps->size);

    return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE mcpkg_deps_unpack(msgpack_object *deps_obj,
                                   McPkgDeps *deps)
{
    if (!deps_obj || !deps)
        return MCPKG_ERROR_OOM;

    if(deps_obj->type != MSGPACK_OBJECT_MAP)
        return MCPKG_ERROR_PARSE;

    msgpack_object_map map = deps_obj->via.map;

    for (size_t i = 0; i < map.size; ++i) {
        msgpack_object key = map.ptr[i].key;
        msgpack_object val = map.ptr[i].val;

        if (equal_key(key, "id")) {
            if (val.type == MSGPACK_OBJECT_STR)
                deps->id = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "name")) {
            if (val.type == MSGPACK_OBJECT_STR)
                deps->name = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "version")) {
            if (val.type == MSGPACK_OBJECT_STR)
                deps->version = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "dependency_type")) {
            if (val.type == MSGPACK_OBJECT_STR)
                deps->dependency_type = strndup(val.via.str.ptr, val.via.str.size);


        } else if (equal_key(key, "loaders")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                if (!deps->loaders)
                    deps->loaders = str_array_new();

                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR) {
                        // TODO DEBUG THIS when I get some free time
                        /* Avoid double-alloc: use length-based add */
                        str_array_add_n(deps->loaders, elem->via.str.ptr, elem->via.str.size);
                    }
                }
            }

        } else if (equal_key(key, "url")) {
            if (val.type == MSGPACK_OBJECT_STR)
                deps->url = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "file_name")) {
            if (val.type == MSGPACK_OBJECT_STR)
                deps->file_name = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "size")) {
            if (val.type == MSGPACK_OBJECT_POSITIVE_INTEGER)
                deps->size = val.via.u64;
        }
    }
    return MCPKG_ERROR_SUCCESS;
}

char *mcpkg_deps_to_string(const McPkgDeps *deps)
{
    if (!deps)
        return strdup("NULL");

    char *loaders_str = deps->loaders ? str_array_to_string(deps->loaders) : strdup("[]");

    char *out = NULL;
    asprintf(&out,
             "Dependencies {\n"
             "  id: %s\n"
             "  name: %s\n"
             "  version: %s\n"
             "  dependency_type: %s\n"
             "  loaders: %s\n"
             "  url: %s\n"
             "  file_name: %s\n"
             "  size: %" PRIu64 "\n"
             "}\n",
             mcpkg_safe_str(deps->id),
             mcpkg_safe_str(deps->name),
             mcpkg_safe_str(deps->version),
             mcpkg_safe_str(deps->dependency_type),
             mcpkg_safe_str(loaders_str),
             mcpkg_safe_str(deps->url),
             mcpkg_safe_str(deps->file_name),
             deps->size);

    free(loaders_str);
    return out;
}
