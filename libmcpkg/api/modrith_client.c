#include "modrith_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <zstd.h>
#include <libgen.h>

#include "mcpkg.h"
#include "utils/code_names.h"
#include "utils/mcpkg_fs.h"
#include "mcpkg_info_entry.h"
#include "mcpkg_cache.h"
#include "mcpkg_get.h"
#include "api/mcpkg_entry.h"
#include "api/mcpkg_deps_entry.h"
#include "utils/array_helper.h"

static McPkgInfoEntry *mcpkg_info_entry_from_cjson(cJSON *mod_item) {
    if (!cJSON_IsObject(mod_item)) return NULL;

    McPkgInfoEntry *entry = mcpkg_info_entry_new();
    if (!entry) return NULL;

    cJSON *temp_json;

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "project_id");
    if (cJSON_IsString(temp_json)) entry->id = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "slug");
    if (cJSON_IsString(temp_json)) entry->name = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "author");
    if (cJSON_IsString(temp_json)) entry->author = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "title");
    if (cJSON_IsString(temp_json)) entry->title = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "description");
    if (cJSON_IsString(temp_json)) entry->description = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "icon_url");
    if (cJSON_IsString(temp_json)) entry->icon_url = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "categories");
    if (cJSON_IsArray(temp_json)) entry->categories = cjson_to_str_array(temp_json);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "versions");
    if (cJSON_IsArray(temp_json)) entry->versions = cjson_to_str_array(temp_json);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "downloads");
    if (cJSON_IsNumber(temp_json)) entry->downloads = (uint32_t)temp_json->valuedouble;

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "date_modified");
    if (cJSON_IsString(temp_json)) entry->date_modified = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "latest_version");
    if (cJSON_IsString(temp_json)) entry->latest_version = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "license");
    if (cJSON_IsString(temp_json)) entry->license = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "client_side");
    if (cJSON_IsString(temp_json)) entry->client_side = strdup(temp_json->valuestring);

    temp_json = cJSON_GetObjectItemCaseSensitive(mod_item, "server_side");
    if (cJSON_IsString(temp_json)) entry->server_side = strdup(temp_json->valuestring);

    return entry;
}


ModrithApiClient *modrith_client_new(const char *mc_version, const char *mod_loader) {
    ModrithApiClient *client_data = calloc(1, sizeof(ModrithApiClient));
    if (!client_data) return NULL;

    client_data->client = api_client_new();
    if (!client_data->client) {
        free(client_data);
        return NULL;
    }

    client_data->mc_version = mc_version ? strdup(mc_version) : NULL;
    client_data->mod_loader = mod_loader ? strdup(mod_loader) : NULL;
    client_data->user_agent = MCPKG_USER_AGENT;

    return client_data;
}

void modrith_client_free(ModrithApiClient *client_data) {
    if (!client_data)
        return;
    api_client_free(client_data->client);
    free(client_data->mc_version);
    free(client_data->mod_loader);
    free(client_data);
}

int modrith_client_update(ModrithApiClient *client_data) {
    size_t offset = 0;
    msgpack_sbuffer sbuf_all_mods;
    msgpack_sbuffer_init(&sbuf_all_mods);

    const char *version_to_use = getenv(ENV_MC_VERSION);
    if (!version_to_use) version_to_use = client_data->mc_version;

    const char *loader_to_use = getenv(ENV_MC_LOADER);
    if (!loader_to_use) loader_to_use = client_data->mod_loader;

    if (!version_to_use || !loader_to_use) {
        fprintf(stderr, "Error: Minecraft version and loader must be specified.\n");
        return MCPKG_ERROR_GENERAL;
    }

    char cache_base_path[1024];
    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root) {
        cache_root = MCPKG_CACHE;
    }

    const char *codename = codename_for_version(version_to_use);
    if (!codename) {
        fprintf(stderr, "Error: Unknown codename for version %s.\n", version_to_use);
        return MCPKG_ERROR_GENERAL;
    }

    snprintf(cache_base_path, sizeof(cache_base_path), "%s/%s/%s/%s",
             cache_root, loader_to_use, codename, version_to_use);

    if (mkdir_p(cache_base_path) != MCPKG_ERROR_SUCCESS) {
        fprintf(stderr, "Failed to create cache directory: %s\n", cache_base_path);
        return MCPKG_ERROR_GENERAL;
    }

    char url[2048];
    CURL *curl = client_data->client->curl;

    while (true) {
        char facets_raw[256];
        snprintf(facets_raw, sizeof(facets_raw), "[[\"categories:%s\"],[\"versions:%s\"]]", loader_to_use, version_to_use);

        char *facets_encoded = curl_easy_escape(curl, facets_raw, 0);
        if (!facets_encoded) {
            fprintf(stderr, "Failed to URL-encode facets.\n");
            return MCPKG_ERROR_GENERAL;
        }

        snprintf(url, sizeof(url), "%s?facets=%s&limit=100&offset=%zu&project_type=mod",
                 MODRINTH_API_SEARCH_URL_BASE, facets_encoded, offset);

        cJSON *json_response = api_client_get(client_data->client, url, NULL);
        if (!json_response) {
            fprintf(stderr, "API call failed for offset %zu\n", offset);
            curl_free(facets_encoded);
            break;
        }

        cJSON *hits_array = cJSON_GetObjectItemCaseSensitive(json_response, "hits");
        if (!cJSON_IsArray(hits_array) || cJSON_GetArraySize(hits_array) == 0) {
            cJSON_Delete(json_response);
            curl_free(facets_encoded);
            break;
        }

        cJSON *mod_item;
        cJSON_ArrayForEach(mod_item, hits_array) {
            McPkgInfoEntry *entry = mcpkg_info_entry_from_cjson(mod_item);
            if (entry) {
                msgpack_sbuffer sbuf_entry;
                msgpack_sbuffer_init(&sbuf_entry);
                if (mcpkg_info_entry_pack(entry, &sbuf_entry) == 0) {
                    msgpack_sbuffer_write(&sbuf_all_mods, sbuf_entry.data, sbuf_entry.size);
                }
                msgpack_sbuffer_destroy(&sbuf_entry);
                mcpkg_info_entry_free(entry);
            }
        }

        cJSON_Delete(json_response);
        curl_free(facets_encoded);
        offset += 100;

        usleep(40 * 1000);
    }

    if (sbuf_all_mods.size == 0) {
        msgpack_sbuffer_destroy(&sbuf_all_mods);
        fprintf(stderr, "No mods found to write.\n");
        return MCPKG_ERROR_NOT_FOUND;
    }

    // uncompressed
    char package_info_path[1024];
    snprintf(package_info_path, sizeof(package_info_path), "%s/Packages.info", cache_base_path);
    FILE *fp_info = fopen(package_info_path, "wb");
    if (fp_info) {
        fwrite(sbuf_all_mods.data, 1, sbuf_all_mods.size, fp_info);
        fclose(fp_info);
        printf("Wrote uncompressed Packages.info to: %s\n", package_info_path);
    } else {
        perror("Failed to write Packages.info");
        msgpack_sbuffer_destroy(&sbuf_all_mods);
        return MCPKG_ERROR_GENERAL;
    }

    // compressed
    char compressed_path[1024];
    snprintf(compressed_path, sizeof(compressed_path), "%s/Packages.info.zstd", cache_base_path);
    if (write_compressed_cache(compressed_path, sbuf_all_mods.data, sbuf_all_mods.size) != MCPKG_ERROR_SUCCESS) {
        fprintf(stderr, "Failed to write compressed cache.\n");
        msgpack_sbuffer_destroy(&sbuf_all_mods);
        return MCPKG_ERROR_GENERAL;
    }

    msgpack_sbuffer_destroy(&sbuf_all_mods);
    return MCPKG_ERROR_SUCCESS;
}

static cJSON *modrith_get_version_by_id(ModrithApiClient *client, const char *version_id) {
    if (!client || !version_id) return NULL;
    char url[1024];
    snprintf(url, sizeof(url), "https://api.modrinth.com/v2/version/%s", version_id);
    cJSON *json = api_client_get(client->client, url, NULL);
    if (!json || !cJSON_IsObject(json)) { if (json) cJSON_Delete(json); return NULL; }
    return json;
}


static int resolve_and_install(ModrithApiClient *client,
                               const char *id_or_slug,
                               const char *mods_dir,
                               const char *install_db,
                               VisitedSet *visited) {
    if (!client || !id_or_slug) return MCPKG_ERROR_PARSE;

    /* If we’ve already processed this project, bail (cycle prevention). */
    if (visited_contains(visited, id_or_slug)) return MCPKG_ERROR_SUCCESS;
    visited_add(visited, id_or_slug);

    /* Try specific version-id if caller passed one (heuristic: Modrinth version IDs are 8 chars of base62). */
    int rc = MCPKG_ERROR_SUCCESS;
    cJSON *version_json = NULL;
    if (strlen(id_or_slug) >= 8 && strlen(id_or_slug) <= 16) {
        version_json = modrith_get_version_by_id(client, id_or_slug);
    }

    McPkgEntry *entry = NULL;

    if (version_json) {
        /* Build entry from exact version */
        if (modrith_version_to_entry(version_json, &entry) != 0 || !entry) {
            cJSON_Delete(version_json);
            return MCPKG_ERROR_PARSE;
        }
        cJSON_Delete(version_json);
    } else {
        /* Otherwise pick latest compatible version for project id/slug */
        cJSON *versions = modrith_get_versions_json(client, id_or_slug);
        if (!versions) return MCPKG_ERROR_NETWORK;

        cJSON *best = modrith_pick_best_version(client, versions);
        if (!best) { cJSON_Delete(versions); return MCPKG_ERROR_NOT_FOUND; }

        if (modrith_version_to_entry(best, &entry) != 0 || !entry) {
            cJSON_Delete(versions);
            return MCPKG_ERROR_PARSE;
        }
        cJSON_Delete(versions);
    }

    /* Resolve required dependencies first */
    for (size_t i = 0; i < entry->dependencies_count; ++i) {
        McPkgDeps *d = entry->dependencies[i];
        if (!d) continue;

        const char *dtype = d->name;           // earlier we stashed dependency_type into name
        const char *proj  = d->id;             // dep project id
        const char *verid = d->version;        // if we later add explicit field for version_id, use it here

        if (!dep_is_required(dtype) || !proj) continue;

        /* If we had version_id from JSON, prefer that exact version.  */
        if (verid && *verid) {
            rc = resolve_and_install(client, verid, mods_dir, install_db, visited);
        } else {
            rc = resolve_and_install(client, proj, mods_dir, install_db, visited);
        }
        if (rc != MCPKG_ERROR_SUCCESS) {
            // You can choose to abort or continue; abort is safer.
            mcpkg_entry_free(entry);
            return rc;
        }
    }

    /* Finally install this entry (upsert + download if needed) */
    rc = install_single_entry(client, mods_dir, install_db, entry); // takes ownership via upsert
    if (rc != MCPKG_ERROR_SUCCESS) {
        mcpkg_entry_free(entry);
        return rc;
    }

    return MCPKG_ERROR_SUCCESS;
}


// GET /v2/project/{id|slug}/version?loaders=[..]&game_versions=[..]
cJSON *modrith_get_versions_json(ModrithApiClient *client, const char *id_or_slug) {
    if (!client || !id_or_slug) return NULL;

    const char *loader = getenv(ENV_MC_LOADER); if (!loader) loader = client->mod_loader;
    const char *mcver  = getenv(ENV_MC_VERSION); if (!mcver)  mcver  = client->mc_version;

    char loaders_q[128]; snprintf(loaders_q, sizeof(loaders_q), "[\"%s\"]", loader ? loader : "");
    char versions_q[128]; snprintf(versions_q, sizeof(versions_q), "[\"%s\"]", mcver ? mcver : "");

    CURL *curl = client->client->curl;
    char *loaders_enc  = curl_easy_escape(curl, loaders_q, 0);
    char *versions_enc = curl_easy_escape(curl, versions_q, 0);
    if (!loaders_enc || !versions_enc) {
        if (loaders_enc) curl_free(loaders_enc);
        if (versions_enc) curl_free(versions_enc);
        return NULL;
    }

    char url[2048];
    snprintf(url, sizeof(url),
             "https://api.modrinth.com/v2/project/%s/version?loaders=%s&game_versions=%s",
             id_or_slug, loaders_enc, versions_enc);

    curl_free(loaders_enc); curl_free(versions_enc);

    cJSON *json = api_client_get(client->client, url, NULL);
    if (!json || !cJSON_IsArray(json)) { if (json) cJSON_Delete(json); return NULL; }
    return json;
}

// Choose latest by date_published
cJSON *modrith_pick_best_version(ModrithApiClient *client, cJSON *versions_array) {
    (void)client;
    if (!versions_array || !cJSON_IsArray(versions_array)) return NULL;
    cJSON *best = NULL;
    const char *best_date = NULL;
    cJSON *item;
    cJSON_ArrayForEach(item, versions_array) {
        cJSON *date = cJSON_GetObjectItemCaseSensitive(item, "date_published");
        if (!cJSON_IsString(date) || !date->valuestring) continue;
        if (!best || strcmp(date->valuestring, best_date) > 0) {
            best = item; best_date = date->valuestring;
        }
    }
    return best;
}

// Convert one version JSON object -> McPkgEntry (includes primary file + minimal deps list)
int modrith_version_to_entry(cJSON *v, McPkgEntry **out) {
    if (!v || !out) return 1;

    McPkgEntry *e = mcpkg_entry_new();
    if (!e) return 1;

    cJSON *tmp;

    // Core fields
    tmp = cJSON_GetObjectItemCaseSensitive(v, "project_id");
    if (cJSON_IsString(tmp)) e->id = strdup(tmp->valuestring);

    tmp = cJSON_GetObjectItemCaseSensitive(v, "name");
    if (cJSON_IsString(tmp)) e->name = strdup(tmp->valuestring); // human-readable version name

    tmp = cJSON_GetObjectItemCaseSensitive(v, "version_number");
    if (cJSON_IsString(tmp)) e->version = strdup(tmp->valuestring);

    tmp = cJSON_GetObjectItemCaseSensitive(v, "date_published");
    if (cJSON_IsString(tmp)) e->date_published = strdup(tmp->valuestring);

    // Arrays
    tmp = cJSON_GetObjectItemCaseSensitive(v, "loaders");
    if (cJSON_IsArray(tmp)) {
        cJSON *it;
        cJSON_ArrayForEach(it, tmp) {
            if (cJSON_IsString(it) && it->valuestring) {
                str_array_add(e->loaders, it->valuestring);
            }
        }
    }

    tmp = cJSON_GetObjectItemCaseSensitive(v, "game_versions");
    if (cJSON_IsArray(tmp)) {
        cJSON *it;
        cJSON_ArrayForEach(it, tmp) {
            if (cJSON_IsString(it) && it->valuestring) {
                str_array_add(e->versions, it->valuestring);
            }
        }
    }

    // Files: choose primary=true; fallback to first
    cJSON *files = cJSON_GetObjectItemCaseSensitive(v, "files");
    if (cJSON_IsArray(files)) {
        cJSON *chosen = NULL, *it;
        cJSON_ArrayForEach(it, files) {
            cJSON *primary = cJSON_GetObjectItemCaseSensitive(it, "primary");
            if (cJSON_IsBool(primary) && cJSON_IsTrue(primary)) { chosen = it; break; }
            if (!chosen) chosen = it;
        }
        if (chosen) {
            cJSON *fn   = cJSON_GetObjectItemCaseSensitive(chosen, "filename");
            cJSON *url  = cJSON_GetObjectItemCaseSensitive(chosen, "url");
            cJSON *size = cJSON_GetObjectItemCaseSensitive(chosen, "size");
            if (cJSON_IsString(fn))   e->file_name = strdup(fn->valuestring);
            if (cJSON_IsString(url))  e->url = strdup(url->valuestring);
            if (cJSON_IsNumber(size)) e->size = (uint64_t)size->valuedouble;

            cJSON *hashes = cJSON_GetObjectItemCaseSensitive(chosen, "hashes");
            if (cJSON_IsObject(hashes)) {
                cJSON *sha512 = cJSON_GetObjectItemCaseSensitive(hashes, "sha512");
                cJSON *sha1   = cJSON_GetObjectItemCaseSensitive(hashes, "sha1");
                if (cJSON_IsString(sha512)) e->sha = strdup(sha512->valuestring);
                else if (cJSON_IsString(sha1)) e->sha = strdup(sha1->valuestring); // fallback
            }
        }
    }

    // Dependencies (now filling dependency_type + version_id properly)
    cJSON *deps = cJSON_GetObjectItemCaseSensitive(v, "dependencies");
    if (cJSON_IsArray(deps)) {
        size_t cap = (size_t)cJSON_GetArraySize(deps);
        if (cap) {
            e->dependencies = (McPkgDeps**)calloc(cap, sizeof(McPkgDeps*));
            if (!e->dependencies) { mcpkg_entry_free(e); return 1; }

            size_t idx = 0;
            cJSON *dobj;
            cJSON_ArrayForEach(dobj, deps) {
                if (!cJSON_IsObject(dobj)) continue;

                McPkgDeps *d = mcpkg_deps_new();
                if (!d) continue;

                cJSON *pid   = cJSON_GetObjectItemCaseSensitive(dobj, "project_id");
                cJSON *vid   = cJSON_GetObjectItemCaseSensitive(dobj, "version_id");
                cJSON *dtype = cJSON_GetObjectItemCaseSensitive(dobj, "dependency_type");

                if (cJSON_IsString(pid))   d->id = strdup(pid->valuestring);
                if (cJSON_IsString(vid))   d->version = strdup(vid->valuestring);         // version_id (if present)
                if (cJSON_IsString(dtype)) d->dependency_type = strdup(dtype->valuestring);

                /* Only keep deps that at least reference a project or a specific version. */
                if (!d->id && !d->version) {
                    mcpkg_deps_free(d);
                    continue;
                }

                e->dependencies[idx++] = d;
            }
            e->dependencies_count = idx;

            /* If none were valid, free the array to keep things tidy. */
            if (idx == 0) {
                free(e->dependencies);
                e->dependencies = NULL;
            }
        }
    }

    *out = e;
    return 0;
}


int modrith_client_install(ModrithApiClient *client, const char *id_or_slug) {
    if (!client || !id_or_slug)
        return MCPKG_ERROR_PARSE;

    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root) cache_root = MCPKG_CACHE;

    const char *loader = getenv(ENV_MC_LOADER);
    if (!loader) loader = client->mod_loader;

    const char *mcver = getenv(ENV_MC_VERSION);
    if (!mcver) mcver = client->mc_version;

    const char *codename = codename_for_version(mcver);
    if (!codename)
        return MCPKG_ERROR_VERSION_MISMATCH;

    // Ensure mods dir exists
    char *mods_dir = NULL;
    if (mods_dir_path(&mods_dir, cache_root, loader, codename, mcver) != 0)
        return MCPKG_ERROR_OOM;
    if (ensure_dir(mods_dir) != 0) {
        free(mods_dir);
        return MCPKG_ERROR_FS;
    }

    // Packages.install path
    char *install_db = NULL;
    if (install_db_path(&install_db, cache_root, loader, codename, mcver) != 0) {
        free(mods_dir);
        return MCPKG_ERROR_OOM;
    }

    // Optional: ensure search cache is present (not strictly required for install)
    McPkgCache *cache = mcpkg_cache_new();
    if (!cache) {
        free(mods_dir);
        free(install_db);
        return MCPKG_ERROR_OOM;
    }
    int rc = mcpkg_cache_load(cache, loader, mcver);
    if (rc != MCPKG_ERROR_SUCCESS) {
        fprintf(stderr, "Warning: search cache not available for %s/%s (continuing install anyway)\n", loader, mcver);
        // Not fatal; continue without cache
    }

    // Resolve and install recursively starting from the target (uses install_single_entry internally)
    VisitedSet vs = (VisitedSet){0};
    int dep_rc = resolve_and_install(client, id_or_slug, mods_dir, install_db, &vs);
    visited_free(&vs);

    // Cleanup
    mcpkg_cache_free(cache);
    free(mods_dir);
    free(install_db);

    return dep_rc;
}
