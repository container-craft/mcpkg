#ifndef MCPKG_CODENAME_H
#define MCPKG_CODENAME_H

#include <stdlib.h>
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

#define ENV_MC_VERSION          "MC_VERSION"
#define MC_DEFAULT_VERSION      "1.21.8"

typedef struct {
    const char *version;
    const char *codename;
} McPkgCodeNameHash;

// A struct to represent a codename and all its associated versions.
typedef struct {
    const char *codename;
    const char **versions;
    size_t count;
} McPkgVersionHash;



const char *mcpkg_mc_codename_from_version(const char *mc_version);
const McPkgVersionHash *mcpkg_mc_versions_from_codename(const char *code_name);

const char *mcpkg_mc_latest_version_from_codename(const char *code_name);

MCPKG_END_DECLS
#endif // MCPKG_CODENAME_H
