#ifndef MCPKG_FS_DIR_H
#define MCPKG_FS_DIR_H

#include "mcpkg_export.h"
#include "fs/mcpkg_fs_error.h"

MCPKG_BEGIN_DECLS

/* return 1 if path is an existing directory, 0 if not, <0 on error. */
MCPKG_API int mcpkg_fs_dir_exists(const char *path);

/* mkdir -p path with MCPKG_FS_DIR_PERM (subject to umask). */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_mkdir_p(const char *path);

/* recursively copy directory tree src -> dst.
 * if overwrite==0, existing files cause ERR_EXISTS.
 */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_cp_dir(const char *src, const char *dst,
                                         int overwrite);

/* recursively remove directory tree (like rm -rf). */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_rm_r(const char *path);

MCPKG_END_DECLS
#endif /* MCPKG_FS_DIR_H */
