#include "mcpkg_info_entry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils/compat.h"
static inline char *strndup_safe(const char *s, size_t n) {
    if (!s || n == 0) return NULL;
    char *out = malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

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

McPkgInfoEntry *mcpkg_info_entry_new(void) {
    return calloc(1, sizeof(McPkgInfoEntry));
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
    if (!entry || !sbuf) return 1;

    msgpack_packer pk;
    msgpack_packer_init(&pk, sbuf, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 14);

    msgpack_pack_str_with_body(&pk, "id", 2);              pack_str_or_nil(&pk, entry->id);
    msgpack_pack_str_with_body(&pk, "name", 4);            pack_str_or_nil(&pk, entry->name);
    msgpack_pack_str_with_body(&pk, "author", 6);          pack_str_or_nil(&pk, entry->author);
    msgpack_pack_str_with_body(&pk, "title", 5);           pack_str_or_nil(&pk, entry->title);
    msgpack_pack_str_with_body(&pk, "description", 11);    pack_str_or_nil(&pk, entry->description);
    msgpack_pack_str_with_body(&pk, "icon_url", 8);        pack_str_or_nil(&pk, entry->icon_url);

    msgpack_pack_str_with_body(&pk, "categories", 10);
    if (entry->categories) str_array_pack(&pk, entry->categories); else msgpack_pack_nil(&pk);

    msgpack_pack_str_with_body(&pk, "versions", 8);
    if (entry->versions)   str_array_pack(&pk, entry->versions);   else msgpack_pack_nil(&pk);

    msgpack_pack_str_with_body(&pk, "downloads", 9);
    msgpack_pack_uint64(&pk, (uint64_t)entry->downloads);

    msgpack_pack_str_with_body(&pk, "date_modified", 13);  pack_str_or_nil(&pk, entry->date_modified);
    msgpack_pack_str_with_body(&pk, "latest_version", 14); pack_str_or_nil(&pk, entry->latest_version);
    msgpack_pack_str_with_body(&pk, "license", 7);         pack_str_or_nil(&pk, entry->license);
    msgpack_pack_str_with_body(&pk, "client_side", 11);    pack_str_or_nil(&pk, entry->client_side);
    msgpack_pack_str_with_body(&pk, "server_side", 11);    pack_str_or_nil(&pk, entry->server_side);

    return 0;
}

int mcpkg_info_entry_unpack(msgpack_object *obj, McPkgInfoEntry *entry) {
    if (!obj || obj->type != MSGPACK_OBJECT_MAP || !entry) return 1;

    msgpack_object_map map = obj->via.map;

    for (size_t i = 0; i < map.size; ++i) {
        msgpack_object key = map.ptr[i].key;
        msgpack_object val = map.ptr[i].val;

        if (equal_key(key, "id")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->id = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "name")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->name = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "author")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->author = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "title")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->title = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "description")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->description = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "icon_url")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->icon_url = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "categories")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                if (!entry->categories) entry->categories = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR)
                        str_array_add_n(entry->categories, elem->via.str.ptr, elem->via.str.size);
                }
            }
        } else if (equal_key(key, "versions")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                if (!entry->versions) entry->versions = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR)
                        str_array_add_n(entry->versions, elem->via.str.ptr, elem->via.str.size);
                }
            }
        } else if (equal_key(key, "downloads")) {
            if (val.type == MSGPACK_OBJECT_POSITIVE_INTEGER)
                entry->downloads = (uint32_t)val.via.u64;
        } else if (equal_key(key, "date_modified")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->date_modified = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "latest_version")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->latest_version = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "license")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->license = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "client_side")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->client_side = strndup_safe(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "server_side")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->server_side = strndup_safe(val.via.str.ptr, val.via.str.size);
        }
    }

    return 0;
}

#define S(x) ((x) ? (x) : "(null)")

char *mcpkg_info_entry_to_string(const McPkgInfoEntry *entry) {
    if (!entry) return strdup("NULL");

    char *categories_str = entry->categories ? str_array_to_string(entry->categories) : strdup("[]");
    char *versions_str   = entry->versions   ? str_array_to_string(entry->versions)   : strdup("[]");

    char *output = NULL;
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
             S(entry->id), S(entry->name), S(entry->author), S(entry->title),
             S(entry->description), entry->downloads,
             S(categories_str), S(versions_str),
             S(entry->date_modified), S(entry->latest_version),
             S(entry->license), S(entry->client_side), S(entry->server_side));

    free(categories_str);
    free(versions_str);
    return output;
}
