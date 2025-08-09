#ifndef MCPKG_CODENAME_H
#define MCPKG_CODENAME_H

#include <stdlib.h>

// A struct to represent a single Minecraft version and its codename.
typedef struct {
    const char *version;
    const char *codename;
} mc_codename_map;

// A struct to represent a codename and all its associated versions.
typedef struct {
    const char *codename;
    const char **versions;
    size_t count;
} mc_versions_by_codename;


/**
 * @brief Returns the codename for a given Minecraft version.
 * @param mc_version The Minecraft version string (e.g., "1.21.8").
 * @return The codename string, or NULL if not found.
 */
const char *codename_for_version(const char *mc_version);

/**
 * @brief Returns all versions associated with a codename.
 * @param code_name The codename string (e.g., "tricky_trials").
 * @return A pointer to a struct containing the versions array and count, or NULL if not found.
 */
const mc_versions_by_codename *versions_for_codename(const char *code_name);

// /**
//  * @brief compare's two different versions
//  * @param v1 version one
//  * @param v2 version two
//  * @return Returns MCPKG_ERROR_SUCCESS if true else MCPKG_ERROR_VERSION_MISMATCH
//  */
// int compare_versions(const char *v1, const char *v2)
/**
 * @brief Returns the latest version for a given codename.
 * @param code_name The codename string.
 * @return The latest version string, or NULL if not found.
 */
const char *latest_version_for_codename(const char *code_name);

#endif // MCPKG_CODENAME_H
