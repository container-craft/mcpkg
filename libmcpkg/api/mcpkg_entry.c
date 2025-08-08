#include "mcpkg_entry.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


McPkgEntry *mcpkg_entry_new() {
    McPkgEntry *entry = calloc(1, sizeof(McPkgEntry));
    if (!entry) return NULL;
    entry->loaders = str_array_new();
    entry->versions = str_array_new();
    entry->dependencies = NULL;
    entry->dependencies_count = 0;
    return entry;
}

void mcpkg_entry_free(McPkgEntry *entry) {
    if (!entry) return;
    free(entry->id);
    free(entry->name);
    free(entry->author);
    free(entry->sha);
    str_array_free(entry->loaders);
    str_array_free(entry->versions);
    free(entry->file_name);
    free(entry->url);
    free(entry->date_published);
    for (size_t i = 0; i < entry->dependencies_count; ++i) {
        mcpkg_deps_free(entry->dependencies[i]);
    }
    free(entry->dependencies);
    free(entry);
}

int mcpkg_entry_pack(const McPkgEntry *entry, msgpack_sbuffer *sbuf) {
    msgpack_packer pk;
    msgpack_packer_init(&pk, sbuf, msgpack_sbuffer_write);

    // Number of fields to pack
    msgpack_pack_map(&pk, 12);

    msgpack_pack_str_with_body(&pk, "id", 2);
    msgpack_pack_str_with_body(&pk, entry->id, strlen(entry->id));
    msgpack_pack_str_with_body(&pk, "name", 4);
    msgpack_pack_str_with_body(&pk, entry->name, strlen(entry->name));
    msgpack_pack_str_with_body(&pk, "author", 6);
    msgpack_pack_str_with_body(&pk, entry->author, strlen(entry->author));
    msgpack_pack_str_with_body(&pk, "sha", 3);
    msgpack_pack_str_with_body(&pk, entry->sha, strlen(entry->sha));

    msgpack_pack_str_with_body(&pk, "loaders", 7);
    str_array_pack(&pk, entry->loaders);

    msgpack_pack_str_with_body(&pk, "url", 3);
    msgpack_pack_str_with_body(&pk, entry->url, strlen(entry->url));

    msgpack_pack_str_with_body(&pk, "versions", 8);
    str_array_pack(&pk, entry->versions);

    msgpack_pack_str_with_body(&pk, "version", 7);
    msgpack_pack_str_with_body(&pk, entry->version, strlen(entry->version));

    msgpack_pack_str_with_body(&pk, "file_name", 9);
    msgpack_pack_str_with_body(&pk, entry->file_name, strlen(entry->file_name));

    msgpack_pack_str_with_body(&pk, "date_published", 14);
    msgpack_pack_str_with_body(&pk, entry->date_published, strlen(entry->date_published));

    msgpack_pack_str_with_body(&pk, "size", 4);
    msgpack_pack_uint64(&pk, entry->size);

    msgpack_pack_str_with_body(&pk, "dependencies", 12);
    msgpack_pack_array(&pk, entry->dependencies_count);
    for(size_t i = 0; i < entry->dependencies_count; ++i) {
        mcpkg_deps_pack(&pk, entry->dependencies[i]);
    }
    return 0;
}

int mcpkg_entry_unpack(msgpack_object *entry_obj, McPkgEntry *entry) {
    if (entry_obj->type != MSGPACK_OBJECT_MAP) return 1;

    msgpack_object_map map = entry_obj->via.map;
    for (size_t i = 0; i < map.size; ++i) {
        msgpack_object key = map.ptr[i].key;
        msgpack_object val = map.ptr[i].val;

        if (key.type != MSGPACK_OBJECT_STR) continue;

        if (strncmp(key.via.str.ptr, "id", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->id = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "name", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->name = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "author", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->author = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "sha", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->sha = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "loaders", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                entry->loaders = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object* elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR) {
                        str_array_add(entry->loaders, elem->via.str.ptr);
                    }
                }
            }
        } else if (strncmp(key.via.str.ptr, "url", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->url = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "versions", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                entry->versions = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object* elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR) {
                        str_array_add(entry->versions, elem->via.str.ptr);
                    }
                }
            }
        } else if (strncmp(key.via.str.ptr, "version", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->version = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "file_name", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->file_name = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "date_published", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->date_published = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "size", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) entry->size = (uint64_t)val.via.u64;
        } else if (strncmp(key.via.str.ptr, "dependencies", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                entry->dependencies_count = val.via.array.size;
                entry->dependencies = malloc(entry->dependencies_count * sizeof(McPkgDeps*));
                for(size_t j = 0; j < entry->dependencies_count; ++j) {
                    msgpack_object* elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_MAP) {
                        McPkgDeps *dep = mcpkg_deps_new();
                        mcpkg_deps_unpack(elem, dep);
                        entry->dependencies[j] = dep;
                    }
                }
            }
        }
    }
    return 0;
}

char *mcpkg_entry_to_string(const McPkgEntry *entry) {
    if (!entry) return strdup("NULL");

    char *output;
    char *loaders_str = str_array_to_string(entry->loaders);
    char *versions_str = str_array_to_string(entry->versions);

    asprintf(&output,
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
             "  size: %lu\n"
             "  dependencies_count: %zu\n"
             "}\n",
             entry->id, entry->name, entry->author, entry->sha,
             loaders_str, entry->url, versions_str, entry->version, entry->file_name,
             entry->date_published, entry->size, entry->dependencies_count);

    free(loaders_str);
    free(versions_str);

    return output;
}
