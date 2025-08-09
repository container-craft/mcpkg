#ifndef MCPKG_INSTALL_H
#define MCPKG_INSTALL_H

#include <stddef.h>
#include <mcpkg.h>
#include <utils/array_helper.h>

/**
 * @brief Install one or more packages (by slug or project_id) for the given MC version/loader.
 * @param mc_version e.g. "1.21.8"
 * @param mod_loader e.g. "fabric"
 * @param packages   list of package slugs/ids; must be non-NULL and count > 0
 * @return MCPKG_ERROR_SUCCESS on full success; error code if any install fails.
 */
mcpkg_error_types install_command(const char *mc_version, const char *mod_loader, str_array *packages);

/**
 * @brief Remove one or more packages (future work; stub for now).
 */
mcpkg_error_types rm_command(const char *mc_version, const char *mod_loader, str_array *packages);

#endif // MCPKG_INSTALL_H
