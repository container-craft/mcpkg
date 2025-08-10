#include "mcpkg_entry.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "utils/mcpkg_msgpack.h"

McPkgEntry *mcpkg_entry_new(void)
{
    McPkgEntry *entry = (McPkgEntry *)calloc(1, sizeof(McPkgEntry));
    if (!entry)
        return NULL;

    entry->loaders = str_array_new();
    entry->versions = str_array_new();
    entry->dependencies = NULL;
    entry->dependencies_count = 0;
    return entry;
}

void mcpkg_entry_free(McPkgEntry *entry)
{
    if (!entry)
        return;

    free(entry->id);
    free(entry->name);
    free(entry->author);
    free(entry->sha);
    str_array_free(entry->loaders);
    str_array_free(entry->versions);
    free(entry->file_name);
    free(entry->url);
    free(entry->version);
    free(entry->date_published);

    // TODO double check the release of this memory when I get a chance.
    if (entry->dependencies) {
        for (size_t i = 0; i < entry->dependencies_count; ++i)
            mcpkg_deps_free(entry->dependencies[i]);

        free(entry->dependencies);
    }
    free(entry);
}

MCPKG_ERROR_TYPE mcpkg_entry_pack(const McPkgEntry *entry,
                     msgpack_sbuffer *sbuf)
{
    if (!entry || !sbuf)
        return MCPKG_ERROR_OOM;

    msgpack_packer pk;
    msgpack_packer_init(&pk, sbuf, msgpack_sbuffer_write);

    /* 12 fields total */
    msgpack_pack_map(&pk, 12);

    msgpack_pack_str_with_body(&pk, "id", 2);
    pack_str_or_nil(&pk, entry->id);

    msgpack_pack_str_with_body(&pk, "name", 4);
    pack_str_or_nil(&pk, entry->name);

    msgpack_pack_str_with_body(&pk, "author", 6);
    pack_str_or_nil(&pk, entry->author);

    msgpack_pack_str_with_body(&pk, "sha", 3);
    pack_str_or_nil(&pk, entry->sha);

    msgpack_pack_str_with_body(&pk, "loaders", 7);
    if (entry->loaders)
        str_array_pack(&pk, entry->loaders);
    else
        msgpack_pack_array(&pk, 0);

    msgpack_pack_str_with_body(&pk, "url", 3);
    pack_str_or_nil(&pk, entry->url);

    msgpack_pack_str_with_body(&pk, "versions", 8);
    if (entry->versions)
        str_array_pack(&pk, entry->versions);
    else
        msgpack_pack_array(&pk, 0);

    msgpack_pack_str_with_body(&pk, "version", 7);
    pack_str_or_nil(&pk, entry->version);

    msgpack_pack_str_with_body(&pk, "file_name", 9);
    pack_str_or_nil(&pk, entry->file_name);

    msgpack_pack_str_with_body(&pk, "date_published", 14);
    pack_str_or_nil(&pk, entry->date_published);

    msgpack_pack_str_with_body(&pk, "size", 4);
    msgpack_pack_uint64(&pk, entry->size);


    // TODO watch this in the debugger later when I get a chance
    msgpack_pack_str_with_body(&pk, "dependencies", 12);
    if (!entry->dependencies || entry->dependencies_count == 0) {
        msgpack_pack_array(&pk, 0);
    } else {
        msgpack_pack_array(&pk, entry->dependencies_count);
        for (size_t i = 0; i < entry->dependencies_count; ++i) {
            mcpkg_deps_pack(&pk, entry->dependencies[i]);
        }
    }

    return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE mcpkg_entry_unpack(msgpack_object *entry_obj,
                                    McPkgEntry *entry)
{
    if (!entry_obj || !entry)
        return MCPKG_ERROR_OOM;

    if(entry_obj->type != MSGPACK_OBJECT_MAP)
        return MCPKG_ERROR_PARSE;


    msgpack_object_map map = entry_obj->via.map;

    for (size_t i = 0; i < map.size; ++i) {
        msgpack_object key = map.ptr[i].key;
        msgpack_object val = map.ptr[i].val;

        if (equal_key(key, "id")) {
            if (val.type == MSGPACK_OBJECT_STR)
                entry->id = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "name")) {
            if (val.type == MSGPACK_OBJECT_STR)
                entry->name = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "author")) {
            if (val.type == MSGPACK_OBJECT_STR)
                entry->author = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "sha")) {
            if (val.type == MSGPACK_OBJECT_STR)
                entry->sha = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "loaders")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {

                if (entry->loaders)
                    str_array_free(entry->loaders);

                entry->loaders = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR)
                        str_array_add_from_msgpack_str(entry->loaders, elem->via.str);
                }
            }

        } else if (equal_key(key, "url")) {
            if (val.type == MSGPACK_OBJECT_STR)
                entry->url = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "versions")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                if (entry->versions)
                    str_array_free(entry->versions);

                entry->versions = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR)
                        str_array_add_from_msgpack_str(entry->versions, elem->via.str);
                }
            }

        } else if (equal_key(key, "version")) {
            if (val.type == MSGPACK_OBJECT_STR)
                entry->version = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "file_name")) {
            if (val.type == MSGPACK_OBJECT_STR)
                entry->file_name = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "date_published")) {
            if (val.type == MSGPACK_OBJECT_STR)
                entry->date_published = strndup(val.via.str.ptr, val.via.str.size);

        } else if (equal_key(key, "size")) {
            if (val.type == MSGPACK_OBJECT_POSITIVE_INTEGER)
                entry->size = val.via.u64;

        // TODO DOUBLE CHECK
        } else if (equal_key(key, "dependencies")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                size_t n = val.via.array.size;
                if (n == 0) {
                    // no deps ?
                } else {
                    entry->dependencies = (McPkgDeps **)calloc(n, sizeof(McPkgDeps *)); // MEMORY
                    if (!entry->dependencies)
                        return MCPKG_ERROR_OOM;

                    entry->dependencies_count = 0;
                    for (size_t j = 0; j < n; ++j) {

                        msgpack_object *elem = &val.via.array.ptr[j];
                        if (elem->type != MSGPACK_OBJECT_MAP) {
                            entry->dependencies[j] = NULL;
                            continue;
                        }
                        McPkgDeps *dep = mcpkg_deps_new(); // MEMORY
                        if (!dep) {
                            entry->dependencies[j] = NULL;
                            continue;
                        }
                        if (mcpkg_deps_unpack(elem, dep) != MCPKG_ERROR_SUCCESS) {
                            mcpkg_deps_free(dep);
                            entry->dependencies[j] = NULL;
                            continue;
                        }
                        entry->dependencies[j] = dep;
                        entry->dependencies_count++;
                    }
                }
            }
        }
    }
    return MCPKG_ERROR_SUCCESS;
}

char *mcpkg_entry_to_string(const McPkgEntry *entry)
{
    if (!entry)
        return strdup("NULL");

    char *loaders_str  = entry->loaders  ? str_array_to_string(entry->loaders)  : strdup("[]");
    char *versions_str = entry->versions ? str_array_to_string(entry->versions) : strdup("[]");

    char *out = NULL;
    appendf(&out,
            "McPkgEntry {\n"
            "  id: %s\n"
            "  name: %s\n"
            "  author: %s\n"
            "  sha: %s\n"
            "  loaders: %s\n"
            "  url: %s\n"
            "  versions: %s\n"
            "  version: %s\n"
            "  file_name: %s\n"
            "  date_published: %s\n"
            "  size: %" PRIu64 "\n"
            "  dependencies_count: %zu\n"
            "}\n",
            mcpkg_safe_str(entry->id),
            mcpkg_safe_str(entry->name),
            mcpkg_safe_str(entry->author),
            mcpkg_safe_str(entry->sha),
            mcpkg_safe_str(loaders_str),
            mcpkg_safe_str(entry->url),
            mcpkg_safe_str(versions_str),
            mcpkg_safe_str(entry->version),
            mcpkg_safe_str(entry->file_name),
            mcpkg_safe_str(entry->date_published),
            entry->size,
            entry->dependencies_count
            );

    free(loaders_str);
    free(versions_str);
    return out;
}
