#ifndef MCPKG_API_MODRITH_CLIENT_H
#define MCPKG_API_MODRITH_CLIENT_H

#include <stddef.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "api/mcpkg_api_client.h"
#include "mcpkg_entry.h"

#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS
/**
 * Structure representing a Modrinth API client instance.
 */
typedef struct {
    ApiClient *client;
    char *mc_version;
    MOD_LOADER_TYPE mod_loader;
    const char *user_agent;
} ModrithApiClient;

ModrithApiClient *modrith_client_new(const char *mc_version,
                                     MOD_LOADER_TYPE mod_loader);


void modrith_client_free(ModrithApiClient *client_data);
MCPKG_ERROR_TYPE modrith_client_update(ModrithApiClient *client_data);
MCPKG_ERROR_TYPE modrith_client_install(ModrithApiClient *client,
                                        const char *id_or_slug);


cJSON *modrith_get_versions_json(ModrithApiClient *client, const char *id_or_slug);
cJSON *modrith_pick_best_version(ModrithApiClient *client, cJSON *versions_array);
int modrith_version_to_entry(cJSON *v, McPkgEntry **out);


MCPKG_END_DECLS
#endif /* MCPKG_API_MODRITH_CLIENT_H */
