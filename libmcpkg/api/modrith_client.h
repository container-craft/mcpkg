#ifndef MODRITH_API_H
#define MODRITH_API_H

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stddef.h>

#include "utils/array_helper.h"
#include "api/mcpkg_api_client.h"

// Forward declaration
typedef struct ModrithApiClient ModrithApiClient;

struct ModrithApiClient {
    ApiClient *client;
    char *mc_version;
    char *mod_loader;
    const char *user_agent;
};

/**
 * @brief Allocates and initializes a new ModrithApiClient struct.
 * @param mc_version The Minecraft version to use by default.
 * @param mod_loader The mod loader to use by default.
 * @return A pointer to the newly created struct, or NULL on failure.
 */
ModrithApiClient *modrith_client_new(const char *mc_version, const char *mod_loader);

/**
 * @brief Frees all memory associated with a ModrithApiClient struct.
 * @param client The client to free.
 */
void modrith_client_free(ModrithApiClient *client);

/**
 * @brief Fetches all available mods from Modrinth for the configured version and loader.
 * @param client The Modrinth API client.
 * @return An array of McPkgInfoEntry structs on success, or NULL on failure.
 */
str_array *modrith_client_update(ModrithApiClient *client);

#endif // MODRITH_API_H
