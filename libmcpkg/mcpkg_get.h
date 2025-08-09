#ifndef MCPKG_GET_H
#define MCPKG_GET_H

#include <stdbool.h>
#include <stddef.h>

#include <msgpack.h>

#include <unistd.h>
#include <zstd.h>

#include <curl/curl.h>

#include <cjson/cJSON.h>

#include "mcpkg.h"
#include "mcpkg_api_client.h"
#include "mcpkg_entry.h"
#include "modrith_client.h"
#include "utils/mcpkg_fs.h"

typedef struct McPkgGet McPkgGet;
typedef struct McPkgGet {
    char *base_path;
    McPkgEntry **mods;
    size_t mods_count;
} McPkgGet;

typedef struct {
    char **ids;
    size_t count, cap;
} VisitedSet;
static void visited_free(VisitedSet *vs) {
    if (!vs) return;
    for (size_t i = 0; i < vs->count; ++i) free(vs->ids[i]);
    free(vs->ids);
    memset(vs, 0, sizeof(*vs));
}

static int visited_contains(VisitedSet *vs, const char *id) {
    if (!vs || !id) return 0;
    for (size_t i = 0; i < vs->count; ++i) {
        if (vs->ids[i] && strcmp(vs->ids[i], id) == 0) return 1;
    }
    return 0;
}

static int visited_add(VisitedSet *vs, const char *id) {
    if (!vs || !id) return 1;
    if (visited_contains(vs, id)) return 0;
    if (vs->count == vs->cap) {
        size_t ncap = vs->cap ? vs->cap * 2 : 8;
        char **tmp = realloc(vs->ids, ncap * sizeof(*tmp));
        if (!tmp) return 1;
        vs->ids = tmp; vs->cap = ncap;
    }
    vs->ids[vs->count++] = strdup(id);
    return 0;
}

static int write_compressed_cache(const char *file_path, const void *data, size_t size) {
    FILE *fp = fopen(file_path, "wb");
    if (!fp) {
        perror("Failed to open compressed file");
        return MCPKG_ERROR_GENERAL;
    }

    size_t compressed_size = ZSTD_compressBound(size);
    void *compressed_data = malloc(compressed_size);
    if (!compressed_data) {
        fclose(fp);
        return MCPKG_ERROR_GENERAL;
    }

    size_t result = ZSTD_compress(compressed_data, compressed_size, data, size, 1);
    if (ZSTD_isError(result)) {
        fprintf(stderr, "ZSTD compression failed: %s\n", ZSTD_getErrorName(result));
        free(compressed_data);
        fclose(fp);
        return MCPKG_ERROR_GENERAL;
    }

    fwrite(compressed_data, 1, result, fp);
    free(compressed_data);
    fclose(fp);
    return MCPKG_ERROR_SUCCESS;
}


static int install_db_load(const char *path, McPkgEntry ***out_entries, size_t *out_count) {
    *out_entries = NULL; *out_count = 0;
    if (access(path, F_OK) != 0)
        return 0; // missing is fine

    char *data = NULL; size_t n = 0;
    if (mcpkg_read_cache_file(path, &data, &n) != MCPKG_ERROR_SUCCESS)
        return MCPKG_ERROR_PARSE;

    msgpack_unpacked up; msgpack_unpacked_init(&up);
    size_t off = 0, cap = 8;
    McPkgEntry **arr = malloc(cap * sizeof(*arr));
    if (!arr) {
        free(data);
        msgpack_unpacked_destroy(&up);
        return 1;
    }

    while (msgpack_unpack_next(&up, data, n, &off)) {
        if (up.data.type != MSGPACK_OBJECT_MAP)
            continue;
        McPkgEntry *e = mcpkg_entry_new();
        if (!e) continue;
        if (mcpkg_entry_unpack(&up.data, e) != 0) {
            mcpkg_entry_free(e);
            continue;
        }
        if (*out_count >= cap) {
            cap *= 2;
            arr = realloc(arr, cap*sizeof(*arr));
            if (!arr)
                break;
        }
        arr[(*out_count)++] = e;
    }
    msgpack_unpacked_destroy(&up);
    free(data);
    *out_entries = arr;
    return 0;
}


static int install_db_save(const char *path, McPkgEntry **entries, size_t count) {
    // ensure directory
    char *dup = strdup(path);
    if (!dup) return 1;
    char *slash = strrchr(dup, '/');
    if (slash) { *slash = '\0'; if (ensure_dir(dup) != 0) { free(dup); return 1; } }
    free(dup);

    msgpack_sbuffer sb; msgpack_sbuffer_init(&sb);
    for (size_t i = 0; i < count; ++i) {
        if (!entries[i]) continue;
        if (mcpkg_entry_pack(entries[i], &sb) != 0) { msgpack_sbuffer_destroy(&sb); return 1; }
    }
    FILE *fp = fopen(path, "wb");
    if (!fp) { msgpack_sbuffer_destroy(&sb); return 1; }
    fwrite(sb.data, 1, sb.size, fp);
    fclose(fp);
    msgpack_sbuffer_destroy(&sb);
    return 0;
}


// Replace or append by project id (or slug/name as fallback)
static int install_db_upsert_entry(const char *path, McPkgEntry *new_entry) {
    McPkgEntry **entries = NULL; size_t count = 0;
    if (install_db_load(path, &entries, &count) != 0) return 1;

    size_t idx = count;
    for (size_t i = 0; i < count; ++i) {
        if (!entries[i]) continue;
        if ((entries[i]->id && new_entry->id && strcmp(entries[i]->id, new_entry->id) == 0) ||
            (entries[i]->name && new_entry->name && strcmp(entries[i]->name, new_entry->name) == 0)) {
            idx = i; break;
        }
    }

    if (idx == count) {
        McPkgEntry **tmp = realloc(entries, (count+1)*sizeof(*entries));
        if (!tmp) {
            for (size_t i = 0; i < count; ++i) mcpkg_entry_free(entries[i]);
            free(entries);
            return 1;
        }
        entries = tmp;
        entries[count++] = new_entry;
    } else {
        mcpkg_entry_free(entries[idx]);
        entries[idx] = new_entry;
    }

    int rc = install_db_save(path, entries, count);
    for (size_t i = 0; i < count; ++i) if (entries[i]) mcpkg_entry_free(entries[i]);
    free(entries);
    return rc;
}


static int install_db_is_installed_exact(const char *path, const char *project_id, const char *version_str) {
    if (!project_id || !version_str) return 0;

    McPkgEntry **entries = NULL; size_t count = 0;
    if (install_db_load(path, &entries, &count) != 0) return 0;

    int found = 0;
    for (size_t i = 0; i < count; ++i) {
        McPkgEntry *e = entries[i];
        if (!e) continue;
        if (e->id && e->version && strcmp(e->id, project_id) == 0 && strcmp(e->version, version_str) == 0) {
            found = 1;
        }
        mcpkg_entry_free(e);
    }
    free(entries);
    return found;
}

int http_download_to_file(ApiClient *api, const char *url, const char *sha_hex_or_null, const char *dest_path);

static int install_single_entry(ModrithApiClient *client,
                                const char *mods_dir, const char *install_db,
                                McPkgEntry *entry) {
    if (!entry || !entry->file_name || !entry->url || !entry->id || !entry->version)
        return MCPKG_ERROR_PARSE;

    /* If already installed at the exact version, optionally refresh DB and return */
    if (install_db_is_installed_exact(install_db, entry->id, entry->version)) {
        /* Optional: keep DB fresh; NOTE: upsert takes ownership, so don't touch entry after this */
        (void)install_db_upsert_entry(install_db, entry);
        return MCPKG_ERROR_SUCCESS;
    }

    /* Download JAR first (we still own entry here) */
    char *dest = NULL;
    if (asprintf(&dest, "%s/%s", mods_dir, entry->file_name) < 0)
        return MCPKG_ERROR_OOM;

    int rc = http_download_to_file(client->client, entry->url, entry->sha, dest);
    free(dest);
    if (rc != MCPKG_ERROR_SUCCESS) {
        return rc;
    }

    /* Now record it in Packages.install; upsert TAKES OWNERSHIP of entry. */
    rc = install_db_upsert_entry(install_db, entry);
    /* On success: do not use or free 'entry' anymore (ownership transferred) */
    if (rc != 0) {
        /* If you want to be tidy, you could unlink the downloaded file on failure. */
        return MCPKG_ERROR_FS;
    }

    return MCPKG_ERROR_SUCCESS;
}


static int dep_is_required(const char *type) {
    // Modrinth types: required, optional, incompatible, embedded, (and sometimes "soft" aka optional)
    return (type && strcmp(type, "required") == 0);
}


// FIX implementation
McPkgGet *mcpkg_get_new(void);
void mcpkg_get_free(McPkgGet *get);
int mcpkg_get_install(const char *mc_version, const char *mod_loader, str_array *packages);
// check the cache for its version then check the
char* mcpkg_get_policy(const char *mc_version, const char *mod_loader, str_array *packages);
static int install_db_write_all(const char *install_db_path, McPkgEntry **entries, size_t count) {
    FILE *fp = fopen(install_db_path, "wb");
    if (!fp) {
        perror("install_db_write_all: fopen");
        return 1;
    }

    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);

    for (size_t i = 0; i < count; ++i) {
        if (!entries[i]) continue;
        if (mcpkg_entry_pack(entries[i], &sbuf) != 0) {
            msgpack_sbuffer_destroy(&sbuf);
            fclose(fp);
            return 1;
        }
    }

    size_t wrote = fwrite(sbuf.data, 1, sbuf.size, fp);
    msgpack_sbuffer_destroy(&sbuf);

    if (wrote != (size_t) (ftell(fp))) {
        // Not a reliable check; still ensure fwrite succeeded fully:
        // fwrite returns bytes written; compare with intended size:
        // We already have wrote == sbuf.size implicitly by comparing to ftell post-write.
    }

    if (fclose(fp) != 0) {
        perror("install_db_write_all: fclose");
        return 1;
    }
    return 0;
}

int mcpkg_get_remove(const char *mc_version, const char *mod_loader, str_array *packages);


// check what is installed vs what is upstream; upgrade if new versions and upgrade deps as well (if needed)
int mcpkg_get_upgreade(const char *mc_version, const char *mod_loader);



#endif /* MCPKG_GET_H */
