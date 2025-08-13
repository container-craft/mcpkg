#ifndef MCPKG_GET_INSTALL_H
#define MCPKG_GET_INSTALL_H

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
MCPKG_ERROR_TYPE install_command(const char *mc_version, const char *mod_loader,
                                 str_array *packages);

/**
 * @brief Remove one or more packages
 */
MCPKG_ERROR_TYPE remove_command(const char *mc_version, const char *mod_loader,
                                str_array *packages);

/**
 * @brief show what is installed vs avaialable
 */
MCPKG_ERROR_TYPE policy_command(const char *mc_version, const char *mod_loader,
                                str_array *packages);
/**
 * @brief upgrade the local installed  mods at a version and loader
 */
MCPKG_ERROR_TYPE upgrade_command(const char *mc_version,
                                 const char *mod_loader);

#endif // MCPKG_GET_INSTALL_H
