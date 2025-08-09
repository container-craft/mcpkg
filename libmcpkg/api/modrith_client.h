#ifndef MCPKG_API_MODRITH_CLIENT_H
#define MCPKG_API_MODRITH_CLIENT_H

#include <stddef.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "api/mcpkg_api_client.h"
#include "mcpkg_entry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure representing a Modrinth API client instance.
 */
typedef struct {
    ApiClient *client;        /**< Underlying API HTTP client */
    char *mc_version;         /**< Default Minecraft version */
    char *mod_loader;         /**< Default mod loader */
    const char *user_agent;   /**< User-Agent string for requests */
} ModrithApiClient;

/**
 * Create a new Modrinth API client.
 *
 * @param mc_version Default Minecraft version string.
 * @param mod_loader Default loader (e.g., "fabric", "forge").
 * @return Pointer to newly allocated ModrithApiClient, or NULL on error.
 */
ModrithApiClient *modrith_client_new(const char *mc_version, const char *mod_loader);

/**
 * Free a Modrinth API client.
 */
void modrith_client_free(ModrithApiClient *client_data);

/**
 * Update local cache (Packages.info + Packages.info.zstd) for given MC version and loader.
 *
 * @param client_data Modrinth API client.
 * @return MCPKG_ERROR_* code.
 */
int modrith_client_update(ModrithApiClient *client_data);

/**
 * Install the latest compatible version of a mod by project ID or slug.
 *
 * @param client Modrinth API client.
 * @param id_or_slug Project ID or slug.
 * @return MCPKG_ERROR_* code.
 */
int modrith_client_install(ModrithApiClient *client, const char *id_or_slug);


/**
 * Fetch versions JSON array for a given project ID or slug filtered by loader and MC version.
 *
 * Caller must free the returned cJSON* with cJSON_Delete().
 *
 * @param client Modrinth API client.
 * @param id_or_slug Project ID or slug.
 * @return cJSON array or NULL on error.
 */
cJSON *modrith_get_versions_json(ModrithApiClient *client, const char *id_or_slug);

/**
 * Choose the best/latest version from a Modrinth versions array.
 *
 * @param client Modrinth API client (unused currently).
 * @param versions_array cJSON array from modrith_get_versions_json().
 * @return cJSON object representing the best version, or NULL if none.
 */
cJSON *modrith_pick_best_version(ModrithApiClient *client, cJSON *versions_array);

/**
 * Convert a Modrinth version JSON object to a McPkgEntry.
 *
 * @param v cJSON version object.
 * @param out Output pointer for allocated McPkgEntry.
 * @return 0 on success, non-zero on error.
 */
int modrith_version_to_entry(cJSON *v, McPkgEntry **out);

#ifdef __cplusplus
}
#endif

#endif /* MCPKG_API_MODRITH_CLIENT_H */
