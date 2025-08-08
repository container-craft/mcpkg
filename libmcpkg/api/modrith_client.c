#include "modrith_client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "mcpkg.h"
#include "mcpkg_info_entry.h"

// The API endpoint for searching projects
#define MODRINTH_API_SEARCH_URL_BASE "https://api.modrinth.com/v2/search"


// A helper function to parse a cJSON object into a str_array.
static str_array *cjson_to_str_array(cJSON *json_array) {
    if (!cJSON_IsArray(json_array)) return NULL;

    str_array *arr = str_array_new();
    if (!arr) return NULL;

    cJSON *item;
    cJSON_ArrayForEach(item, json_array) {
        if (cJSON_IsString(item)) {
            str_array_add(arr, item->valuestring);
        }
    }
    return arr;
}

// New helper function to create an McPkgInfoEntry from cJSON
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
    if (cJSON_IsNumber(temp_json)) entry->downloads = (uint32_t)temp_json->valueint;

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

static bool write_mod_to_file(const McPkgInfoEntry *entry, const char *file_path) {
    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);

    if (mcpkg_info_entry_pack(entry, &sbuf) != 0) {
        msgpack_sbuffer_destroy(&sbuf);
        return false;
    }

    FILE *fp = fopen(file_path, "wb");
    if (!fp) {
        msgpack_sbuffer_destroy(&sbuf);
        return false;
    }

    fwrite(sbuf.data, 1, sbuf.size, fp);
    fclose(fp);
    msgpack_sbuffer_destroy(&sbuf);
    return true;
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
    if (!client_data) return;
    api_client_free(client_data->client);
    free(client_data->mc_version);
    free(client_data->mod_loader);
    free(client_data);
}

str_array *modrith_client_update(ModrithApiClient *client_data) {
    size_t offset = 0;
    str_array *all_mods = str_array_new();
    if (!all_mods) return NULL;

    const char *version_to_use = getenv(ENV_MC_VERSION);
    if (!version_to_use) version_to_use = client_data->mc_version;

    const char *loader_to_use = getenv(ENV_MC_LOADER);
    if (!loader_to_use) loader_to_use = client_data->mod_loader;

    if (!version_to_use || !loader_to_use) {
        fprintf(stderr, "Error: Minecraft version and loader must be specified.\n");
        str_array_free(all_mods);
        return NULL;
    }

    char url[2048];
    CURL *curl = client_data->client->curl;

    while (true) {
        // Build the facets string
        char facets_raw[256];
        snprintf(facets_raw, sizeof(facets_raw), "[[\"categories:%s\"],[\"versions:%s\"]]", loader_to_use, version_to_use);

        // URL-encode the facets string
        char *facets_encoded = curl_easy_escape(curl, facets_raw, 0);
        if (!facets_encoded) {
            fprintf(stderr, "Failed to URL-encode facets.\n");
            break;
        }

        // Construct the full URL
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
                // For now, let's write to a temporary file
                char tmp_path[1024];
                snprintf(tmp_path, sizeof(tmp_path), "/tmp/mcpkg_modrith_%s.msgpack", entry->id);
                if (write_mod_to_file(entry, tmp_path)) {
                    printf("Wrote mod info for '%s' to %s\n", entry->title, tmp_path);
                }
                mcpkg_info_entry_free(entry);
            }
        }

        cJSON_Delete(json_response);
        curl_free(facets_encoded);
        offset += 100;

        // Respecting the rate limit of 25 calls/second (40ms)
        usleep(40 * 1000);
    }

    return all_mods;
}
