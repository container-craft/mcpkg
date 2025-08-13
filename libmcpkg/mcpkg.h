#ifndef MCPKG_H
#define MCPKG_H



#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

#define MCPKG_VERSION "0.1"

#define ENV_MCPKG_CACHE         "MCPKG_CACHE"
#ifndef MCPKG_CACHE
#define MCPKG_CACHE         "/var/cache/mcpkg/"
#endif

#define ENV_MC_BASE             "MC_BASE"




MCPKG_END_DECLS
#endif // MCPKG_H
