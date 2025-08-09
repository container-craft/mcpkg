#ifndef MCPKG_H
#define MCPKG_H

#include <msgpack.h>

#define MCPKG_VERSION "0.1"

#ifndef MCPKG_USER_AGENT
#define MCPKG_USER_AGENT "m_jimmer/mcpkg/" MCPKG_VERSION
#endif

#ifndef MODRINTH_API_SEARCH_URL_BASE
#define MODRINTH_API_SEARCH_URL_BASE "https://api.modrinth.com/v2/search"
#endif

#ifndef MCPKG_CACHE
#define MCPKG_CACHE         "/var/cache/mcpkg/"
#endif

// --- Environment Variable Keys ---

#define ENV_MC_VERSION          "MC_VERSION"
#define ENV_MC_LOADER           "MC_LOADER"
#define ENV_MCPKG_CACHE         "MCPKG_CACHE"

// --- Mod Loader Definitions ---

typedef enum {
    MOD_LOADER_FABRIC,
    MOD_LOADER_FORGE,
    MOD_LOADER_NEOFORGE,
    MOD_LOADER_PAPER,
    MOD_LOADER_VELOCITY,
    MOD_LOADER_UNKNOWN
} mod_loader_type;

// --- Provider Definitions ---

typedef enum {
    PROVIDER_MODRITH,
    PROVIDER_CURSEFORGE,
    PROVIDER_HANGAR,
    PROVIDER_LOCAL
} provider_type;

// --- Error Types ---

typedef enum {
    MCPKG_ERROR_SUCCESS,
    MCPKG_ERROR_GENERAL,
    MCPKG_ERROR_NOT_FOUND,
    MCPKG_ERROR_NETWORK,
    MCPKG_ERROR_PARSE,
    MCPKG_ERROR_FS,
    MCPKG_ERROR_VERSION_MISMATCH,
    MCPKG_ERROR_OOM
} mcpkg_error_types;

// --- Main Function Prototypes ---

/**
 * @brief Initializes the mcpkg environment and configuration.
 * @return 0 on success, non-zero on failure.
 */
int mcpkg_init();

#endif // MCPKG_H
