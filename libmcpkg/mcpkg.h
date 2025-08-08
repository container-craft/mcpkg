#ifndef MCPKG_H
#define MCPKG_H

// Later we will make the full include.
// #include "mcpkg_api_client.h" // Includes the API client logic
// #include "mcpkg_api.h"        // Includes the plugin API interface
// #include "code_names.h"         // Includes the version/codename mapping logic

// --- Global Constants and Defines ---

#define MCPKG_VERSION "0.1"
#define MCPKG_USER_AGENT "m_jimmer/mcpkg/0.1.0"

// --- Environment Variable Keys ---

#define ENV_MC_VERSION          "MC_VERSION"
#define ENV_MC_LOADER           "MC_LOADER"
#define ENV_MC_WORK_DIR         "MC_WORK_DIR"
#define ENV_MC_BUILD_DIR        "MC_BUILD_DIR"
#define ENV_MC_CACHE_DIR        "MC_CACHE_DIR"
#define ENV_MC_REPO_DIR         "MC_REPO_DIR"

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

// --- Function Prototypes for main logic later ---

/**
 * @brief Initializes the mcpkg environment and configuration.
 * @return 0 on success, non-zero on failure.
 */
int mcpkg_init();

#endif // MCPKG_H
