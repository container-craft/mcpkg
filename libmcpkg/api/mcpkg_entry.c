#include "mcpkg_entry.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h> /* PRIu64 */

#define S(x) ((x) ? (x) : "(null)")

/* -------- helpers -------- */

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

static int appendf(char **dst, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *add = NULL;
    int nw = vasprintf(&add, fmt, ap);
    va_end(ap);
    if (nw < 0) return -1;

    if (!*dst) {
        *dst = add;
    } else {
        size_t oldlen = strlen(*dst);
        char *tmp = (char *)realloc(*dst, oldlen + (size_t)nw + 1);
        if (!tmp) { free(add); return -1; }
        memcpy(tmp + oldlen, add, (size_t)nw + 1);
        free(add);
        *dst = tmp;
    }
    return 0;
}

/* msgpack string -> add to str_array (without leaking) */
static void str_array_add_from_msgpack_str(str_array *arr, const msgpack_object_str s) {
    if (!arr || !s.ptr || s.size == 0) return;
    char *tmp = (char *)malloc(s.size + 1);
    if (!tmp) return;
    memcpy(tmp, s.ptr, s.size);
    tmp[s.size] = '\0';
    /* str_array_add duplicates; free our temp afterwards */
    str_array_add(arr, tmp);
    free(tmp);
}

/* -------- API -------- */

McPkgEntry *mcpkg_entry_new(void) {
    McPkgEntry *entry = (McPkgEntry *)calloc(1, sizeof(McPkgEntry));
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
    free(entry->version);
    free(entry->date_published);
    if (entry->dependencies) {
        for (size_t i = 0; i < entry->dependencies_count; ++i)
            mcpkg_deps_free(entry->dependencies[i]);
        free(entry->dependencies);
    }
    free(entry);
}

int mcpkg_entry_pack(const McPkgEntry *entry, msgpack_sbuffer *sbuf) {
    if (!entry || !sbuf) return 1;

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
    if (entry->loaders) str_array_pack(&pk, entry->loaders); else msgpack_pack_array(&pk, 0);

    msgpack_pack_str_with_body(&pk, "url", 3);
    pack_str_or_nil(&pk, entry->url);

    msgpack_pack_str_with_body(&pk, "versions", 8);
    if (entry->versions) str_array_pack(&pk, entry->versions); else msgpack_pack_array(&pk, 0);

    msgpack_pack_str_with_body(&pk, "version", 7);
    pack_str_or_nil(&pk, entry->version);

    msgpack_pack_str_with_body(&pk, "file_name", 9);
    pack_str_or_nil(&pk, entry->file_name);

    msgpack_pack_str_with_body(&pk, "date_published", 14);
    pack_str_or_nil(&pk, entry->date_published);

    msgpack_pack_str_with_body(&pk, "size", 4);
    msgpack_pack_uint64(&pk, entry->size);

    msgpack_pack_str_with_body(&pk, "dependencies", 12);
    if (!entry->dependencies || entry->dependencies_count == 0) {
        msgpack_pack_array(&pk, 0);
    } else {
        msgpack_pack_array(&pk, entry->dependencies_count);
        for (size_t i = 0; i < entry->dependencies_count; ++i) {
            mcpkg_deps_pack(&pk, entry->dependencies[i]);
        }
    }

    return 0;
}

int mcpkg_entry_unpack(msgpack_object *entry_obj, McPkgEntry *entry) {
    if (!entry_obj || entry_obj->type != MSGPACK_OBJECT_MAP || !entry) return 1;

    msgpack_object_map map = entry_obj->via.map;

    for (size_t i = 0; i < map.size; ++i) {
        msgpack_object key = map.ptr[i].key;
        msgpack_object val = map.ptr[i].val;

        if (equal_key(key, "id")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->id = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "name")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->name = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "author")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->author = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "sha")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->sha = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "loaders")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                if (entry->loaders) str_array_free(entry->loaders);
                entry->loaders = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR)
                        str_array_add_from_msgpack_str(entry->loaders, elem->via.str);
                }
            }
        } else if (equal_key(key, "url")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->url = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "versions")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                if (entry->versions) str_array_free(entry->versions);
                entry->versions = str_array_new();
                for (size_t j = 0; j < val.via.array.size; ++j) {
                    msgpack_object *elem = &val.via.array.ptr[j];
                    if (elem->type == MSGPACK_OBJECT_STR)
                        str_array_add_from_msgpack_str(entry->versions, elem->via.str);
                }
            }
        } else if (equal_key(key, "version")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->version = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "file_name")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->file_name = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "date_published")) {
            if (val.type == MSGPACK_OBJECT_STR) entry->date_published = strndup(val.via.str.ptr, val.via.str.size);
        } else if (equal_key(key, "size")) {
            if (val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) entry->size = val.via.u64;
        } else if (equal_key(key, "dependencies")) {
            if (val.type == MSGPACK_OBJECT_ARRAY) {
                size_t n = val.via.array.size;
                if (n == 0) {
                    /* nothing to do */
                } else {
                    entry->dependencies = (McPkgDeps **)calloc(n, sizeof(McPkgDeps *));
                    if (!entry->dependencies) return 1;
                    entry->dependencies_count = 0;
                    for (size_t j = 0; j < n; ++j) {
                        msgpack_object *elem = &val.via.array.ptr[j];
                        if (elem->type != MSGPACK_OBJECT_MAP) {
                            entry->dependencies[j] = NULL;
                            continue;
                        }
                        McPkgDeps *dep = mcpkg_deps_new();
                        if (!dep) {
                            entry->dependencies[j] = NULL;
                            continue;
                        }
                        if (mcpkg_deps_unpack(elem, dep) != 0) {
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
    return 0;
}

char *mcpkg_entry_to_string(const McPkgEntry *entry) {
    if (!entry) return strdup("NULL");

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
            S(entry->id), S(entry->name), S(entry->author), S(entry->sha),
            S(loaders_str), S(entry->url), S(versions_str), S(entry->version),
            S(entry->file_name), S(entry->date_published), entry->size,
            entry->dependencies_count);

    free(loaders_str);
    free(versions_str);
    return out;
}
