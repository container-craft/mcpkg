#ifndef MCPKG_TOOL_CONFIG
#define MCPKG_TOOL_CONFIG
#include <mcpkg.h>
#include <mcpkg_config.h>

MCPKG_ERROR_TYPE set_global_mc_base(const char *mc_base);
MCPKG_ERROR_TYPE set_global_mc_loader(const char *mc_loader);
MCPKG_ERROR_TYPE set_global_mc_version(const char *mc_version);

#endif //MCPKG_TOOL_CONFIG
