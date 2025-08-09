#ifndef MCPKG_FS_H
#define MCPKG_FS_H

#include <stddef.h>
#include "mcpkg.h"

/**
 * Recursively creates directories (like `mkdir -p`).
 * Returns MCPKG_ERROR_SUCCESS on success, MCPKG_ERROR_FS on failure.
 */
mcpkg_error_types mkdir_p(const char *path);

/**
 * Creates the file if missing, and updates its atime/mtime.
 */
mcpkg_error_types touch(const char *path);

/**
 * Creates a symlink. If overwrite is true, replaces existing file/link.
 */
mcpkg_error_types create_symlink(const char *target, const char *link_path);
mcpkg_error_types create_symlink_overwrite(const char *target, const char *link_path, int overwrite);

/**
 * Reads an entire file into a newly-allocated buffer.
 * On success: *buffer points to heap data (caller free), *size set to byte count.
 */
mcpkg_error_types mcpkg_read_cache_file(const char *path, char **buffer, size_t *size);



// mods dir and install DB paths
static int mods_dir_path(char **out_path, const char *root, const char *loader, const char *codename, const char *version) {
    if (asprintf(out_path, "%s/%s/%s/%s/mods", root, loader, codename, version) < 0) return 1;
    return 0;
}
static int install_db_path(char **out_path, const char *root, const char *loader, const char *codename, const char *version) {
    if (asprintf(out_path, "%s/%s/%s/%s/mods/Packages.install", root, loader, codename, version) < 0) return 1;
    return 0;
}

static int ensure_dir(const char *p) {
    return (mkdir_p(p) == MCPKG_ERROR_SUCCESS) ? 0 : 1;
}

mcpkg_error_types mcpkg_copy_tree(const char *src, const char *dst);


mcpkg_error_types mcpkg_remove_tree(const char *path);

mcpkg_error_types mcpkg_unlink_any(const char *path);

#endif // MCPKG_FS_H
