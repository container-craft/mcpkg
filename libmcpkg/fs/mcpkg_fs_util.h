#ifndef MCPKG_FS_UTIL_H
#define MCPKG_FS_UTIL_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "fs/mcpkg_fs_error.h"

MCPKG_BEGIN_DECLS

/* default perms (octal). callers may ignore and rely on umask. */
#ifndef MCPKG_FS_DIR_PERM
#define MCPKG_FS_DIR_PERM  0755
#endif
#ifndef MCPKG_FS_FILE_PERM
#define MCPKG_FS_FILE_PERM 0644
#endif

/* path helpers (allocate result with malloc; caller frees) */
MCPKG_API int mcpkg_fs_is_separator(char c);

/* out = join(a, b) using '/'; avoids duplicate separators. */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_join2(const char *a, const char *b,
                                        char **out);

/* convenience builders used by mcpkg */
MCPKG_API MCPKG_FS_ERROR
mcpkg_fs_path_mods_dir(char **out_path,
                       const char *root,
                       const char *loader,
                       const char *codename,
                       const char *version);

MCPKG_API MCPKG_FS_ERROR
mcpkg_fs_path_db_file(char **out_path,
                      const char *root,
                      const char *loader,
                      const char *codename,
                      const char *version); /* .../mods/Packages.install */

/* user config locations (malloc'd) */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_config_dir(char **out_dir);
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_config_file(char **out_file);

MCPKG_END_DECLS
#endif /* MCPKG_FS_UTIL_H */
