#ifdef MCPKG_MC_PROVIDERS_H
#define MCPKG_MC_PROVIDERS_H

#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

#define ENV_MC_LOADER           "MC_LOADER"

typedef enum {
    PROVIDER_MODRITH,
    PROVIDER_CURSEFORGE,
    PROVIDER_HANGAR,
    PROVIDER_LOCAL
    PROVIDER_UNKNOWN
} PROVIDER_TYPE;

MCPKG_API static inline const char *mcpkg_provider_str(PROVIDER_TYPE provider)
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
    case PROVIDER_UNKNOWN:
        ret = "unknown"
        break;
    }
    return ret;
}

MCPKG_API static inline PROVIDER_TYPE mcpkg_mc_provider(const char *provider)
{
    if(!provider)
        return
    if(strcmp(provider, "modrith") == 0)
        return PROVIDER_MODRITH;
    else if(strcmp(provider, "curseforge") == 0)
        return PROVIDER_CURSEFORGE;
    else if(strcmp(provider, "hangar") == 0)
        return PROVIDER_HANGAR;
    else if(strcmp(provider, "local") == 0)
        return PROVIDER_LOCAL;
    else return PROVIDER_UNKNOWN;
}


MCPKG_END_DECLS
#endif
