#ifndef MCPKG_H
#define MCPKG_H

#include <msgpack.h>
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

#define MCPKG_VERSION "0.1"

#ifndef MCPKG_USER_AGENT
#define MCPKG_USER_AGENT "m_jimmer/mcpkg/" MCPKG_VERSION
#endif


#ifndef MODRINTH_API_SEARCH_URL_BASE
#define MODRINTH_API_SEARCH_URL_BASE "https://api.modrinth.com/v2/search"
#endif

#ifndef MODRINTH_API_VERSION_URL_BASE
#define MODRINTH_API_VERSION_URL_BASE "https://api.modrinth.com/v2/version/"
#endif

#ifndef MCPKG_CACHE
#define MCPKG_CACHE         "/var/cache/mcpkg/"
#endif


#define ENV_MC_BASE             "MC_BASE"


#define ENV_MC_VERSION          "MC_VERSION"
#define MC_DEFAULT_VERSION      "1.21.8"

#define ENV_MC_LOADER           "MC_LOADER"
#define ENV_MCPKG_CACHE         "MCPKG_CACHE"

#ifndef MCPKG_SAFE_STR
    #define MCPKG_SAFE_STR(x) ((x) ? (x) : "(null)")
#endif


typedef enum {
    MOD_LOADER_FABRIC,
    MOD_LOADER_FORGE,
    MOD_LOADER_NEOFORGE,
    MOD_LOADER_PAPER,
    MOD_LOADER_VELOCITY,
    MOD_LOADER_UNKNOWN,
} MOD_LOADER_TYPE;


static inline MOD_LOADER_TYPE mcpkg_modloader_from_str(const char *loader)
{
    if(strcmp(loader, "fabric") == 0)
        return MOD_LOADER_FABRIC;
    else if(strcmp(loader, "forge") == 0)
        return MOD_LOADER_FORGE;
    else if(strcmp(loader, "neoforge") == 0)
        return MOD_LOADER_NEOFORGE;
    else if(strcmp(loader, "paper") == 0)
        return MOD_LOADER_PAPER;
    else if(strcmp(loader, "velocity") == 0)
        return MOD_LOADER_VELOCITY;
    return MOD_LOADER_UNKNOWN;
}
static inline const char *mcpkg_modloader_str(MOD_LOADER_TYPE loader)
{
    const char *ret = "unknown";
    switch (loader) {
    case MOD_LOADER_FABRIC:
        ret = "fabric";
        break;
    case MOD_LOADER_FORGE:
        ret = "forge";
        break;
    case MOD_LOADER_NEOFORGE:
        ret = "neoforge";
        break;
    case MOD_LOADER_PAPER:
        ret = "paper";
        break;
    case MOD_LOADER_VELOCITY:
        ret = "velocity";
        break;
    case MOD_LOADER_UNKNOWN:
        ret = "unknown";
        break;
    default:
        ret = "unknown";
        break;
    }
    return ret;
}



typedef enum {
    PROVIDER_MODRITH,
    PROVIDER_CURSEFORGE,
    PROVIDER_HANGAR,
    PROVIDER_LOCAL
} PROVIDER_TYPE;
static inline const char *mcpkg_provider_str(PROVIDER_TYPE provider)
{
    const char *ret = "unknown provider";
    switch (provider) {
    case PROVIDER_MODRITH:
        ret = "modrith";
        break;
    case PROVIDER_CURSEFORGE:
        ret = "curseforge";
        break;
    case PROVIDER_HANGAR:
        ret = "hangar";
        break;
    case PROVIDER_LOCAL:
        ret = "local";
        break;
    }
    return ret;
}

typedef enum {
    MCPKG_ERROR_SUCCESS,
    MCPKG_ERROR_GENERAL,
    MCPKG_ERROR_NOT_FOUND,
    MCPKG_ERROR_NETWORK,
    MCPKG_ERROR_PARSE,
    MCPKG_ERROR_FS,
    MCPKG_ERROR_VERSION_MISMATCH,
    MCPKG_ERROR_OOM,
    MCPKG_ERROR_OOM_MISMATCH
} MCPKG_ERROR_TYPE;
static inline const char *mcpkg_error_str(MCPKG_ERROR_TYPE error)
{
    const char *ret = "unknown error";
    switch (error) {
    case MCPKG_ERROR_SUCCESS:
        ret = "No Error";
        break;
    case MCPKG_ERROR_GENERAL:
        ret = "False";
        break;
    case MCPKG_ERROR_NOT_FOUND:
        ret = "Search yeilds nothing";
        break;
    case MCPKG_ERROR_NETWORK:
        ret = "General Networking issue";
        break;
    case MCPKG_ERROR_PARSE:
        ret = "Parsing issue with api";
        break;
    case MCPKG_ERROR_FS:
        ret = "Files system error.";
        break;
    case MCPKG_ERROR_VERSION_MISMATCH:
        ret = "Requested Minecraft versions to not match or are invaild";
        break;
    case MCPKG_ERROR_OOM:
        ret = "Memory issue something is NULL";
        break;
    case MCPKG_ERROR_OOM_MISMATCH:
        ret = "Memory issue memory is not equal to test";
        break;
    }
    return ret;
}



MCPKG_END_DECLS
#endif // MCPKG_H
