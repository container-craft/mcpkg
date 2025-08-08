#include "mcpkg_info_entry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

McPkgInfoEntry *mcpkg_info_entry_new() {
    McPkgInfoEntry *entry = calloc(1, sizeof(McPkgInfoEntry));
    if (!entry) return NULL;
    entry->categories = str_array_new();
    entry->versions = str_array_new();
    return entry;
}

void mcpkg_info_entry_free(McPkgInfoEntry *entry) {
    if (!entry) return;
    free(entry->id);
    free(entry->name);
    free(entry->author);
    free(entry->title);
    free(entry->description);
    free(entry->icon_url);
    str_array_free(entry->categories);
    str_array_free(entry->versions);
    free(entry->date_modified);
    free(entry->latest_version);
    free(entry->license);
    free(entry->client_side);
    free(entry->server_side);
    free(entry);
}


int mcpkg_info_entry_pack(const McPkgInfoEntry *entry, msgpack_sbuffer *sbuf) {
    msgpack_packer pk;
    msgpack_packer_init(&pk, sbuf, msgpack_sbuffer_write);

    // Number of fields to pack
    msgpack_pack_map(&pk, 14);

    msgpack_pack_str_with_body(&pk, "id", 2);
    msgpack_pack_str_with_body(&pk, entry->id, strlen(entry->id));
    msgpack_pack_str_with_body(&pk, "name", 4);
    msgpack_pack_str_with_body(&pk, entry->name, strlen(entry->name));
    msgpack_pack_str_with_body(&pk, "author", 6);
    msgpack_pack_str_with_body(&pk, entry->author, strlen(entry->author));
    msgpack_pack_str_with_body(&pk, "title", 5);
    msgpack_pack_str_with_body(&pk, entry->title, strlen(entry->title));
    msgpack_pack_str_with_body(&pk, "description", 11);
    msgpack_pack_str_with_body(&pk, entry->description, strlen(entry->description));
    msgpack_pack_str_with_body(&pk, "icon_url", 8);
    msgpack_pack_str_with_body(&pk, entry->icon_url, strlen(entry->icon_url));

    msgpack_pack_str_with_body(&pk, "categories", 10);
    str_array_pack(&pk, entry->categories);

    msgpack_pack_str_with_body(&pk, "versions", 8);
    str_array_pack(&pk, entry->versions);

    msgpack_pack_str_with_body(&pk, "downloads", 9);
    msgpack_pack_uint32(&pk, entry->downloads);

    msgpack_pack_str_with_body(&pk, "date_modified", 13);
    msgpack_pack_str_with_body(&pk, entry->date_modified, strlen(entry->date_modified));

    msgpack_pack_str_with_body(&pk, "latest_version", 14);
    msgpack_pack_str_with_body(&pk, entry->latest_version, strlen(entry->latest_version));

    msgpack_pack_str_with_body(&pk, "license", 7);
    msgpack_pack_str_with_body(&pk, entry->license, strlen(entry->license));

    msgpack_pack_str_with_body(&pk, "client_side", 11);
    msgpack_pack_str_with_body(&pk, entry->client_side, strlen(entry->client_side));

    msgpack_pack_str_with_body(&pk, "server_side", 11);
    msgpack_pack_str_with_body(&pk, entry->server_side, strlen(entry->server_side));

    return 0;
}

int mcpkg_info_entry_unpack(msgpack_object *entry_obj, McPkgInfoEntry *entry) {
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
        } else if (strncmp(key.via.str.ptr, "title", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->title = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "description", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->description = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "icon_url", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->icon_url = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "categories", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                entry->categories = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object* elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR) {
                        str_array_add(entry->categories, elem->via.str.ptr);
                    }
                }
            }
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
        } else if (strncmp(key.via.str.ptr, "downloads", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) entry->downloads = (uint32_t)val.via.u64;
        } else if (strncmp(key.via.str.ptr, "date_modified", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->date_modified = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "latest_version", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->latest_version = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "license", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->license = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "client_side", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->client_side = strndup(val.via.str.ptr, val.via.str.size);
        } else if (strncmp(key.via.str.ptr, "server_side", key.via.str.size) == 0) {
            if (val.type == MSGPACK_OBJECT_STR) entry->server_side = strndup(val.via.str.ptr, val.via.str.size);
        }
    }
    return 0;
}

char *mcpkg_info_entry_to_string(const McPkgInfoEntry *entry) {
    if (!entry) return strdup("NULL");

    char *output;
    char *categories_str = str_array_to_string(entry->categories);
    char *versions_str = str_array_to_string(entry->versions);

    asprintf(&output,
             "McPkgInfoEntry {\n"
             "  id: %s\n"
             "  name: %s\n"
             "  author: %s\n"
             "  title: %s\n"
             "  description: %s\n"
             "  downloads: %u\n"
             "  categories: %s\n"
             "  versions: %s\n"
             "  date_modified: %s\n"
             "  latest_version: %s\n"
             "  license: %s\n"
             "  client_side: %s\n"
             "  server_side: %s\n"
             "}\n",
             entry->id, entry->name, entry->author, entry->title, entry->description, entry->downloads,
             categories_str, versions_str, entry->date_modified, entry->latest_version,
             entry->license, entry->client_side, entry->server_side);

    free(categories_str);
    free(versions_str);

    return output;
}
