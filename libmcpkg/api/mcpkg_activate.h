#ifndef MCPKG_ACTIVATE
#define MCPKG_ACTIVATE
#include <string.h>
#include "mcpkg.h"
#include "utils/mcpkg_export.h"
MCPKG_BEGIN_DECLS

static const char *target_dir_for_loader(const char *loader) {
    if (!loader) return "mods";
    if (strcmp(loader, "fabric") == 0) return "mods";
    return "plugins"; // Paper/Velocity/etc.
}
MCPKG_ERROR_TYPE mcpkg_activate(const char *mc_version, const char *mod_loader);
MCPKG_ERROR_TYPE mcpkg_deactivate(const char *mc_version, const char *mod_loader);

MCPKG_END_DECLS
#endif //MCPKG_ACTIVATE
