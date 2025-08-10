#ifndef CACHE_H
#define CACHE_H

/**
 * @brief Searches the local cache for packages.
 * @param mc_version The Minecraft version to search within.
 * @param mod_loader The mod loader to search for.
 * @param package_name The package name to search for.
 * @return 0 on success, or a MCPKG_ERROR_TYPE code on failure.
 */
int search_cache_command(const char *mc_version, const char *mod_loader, const char *package_name);

/**
 * @brief Displays detailed information for a single package from the local cache.
 * @param mc_version The Minecraft version of the package.
 * @param mod_loader The mod loader of the package.
 * @param package_name The exact package name to show.
 * @return 0 on success, or a MCPKG_ERROR_TYPE code on failure.
 */
int show_cache_command(const char *mc_version, const char *mod_loader, const char *package_name);

#endif // CACHE_H
