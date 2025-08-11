#ifndef MCPKG_MC_LOADERS_H
#define MCPKG_MC_LOADERS_H

#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

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
MCPKG_BEGIN_DECLS
#endif // MCPKG_MC_LOADERS_H
