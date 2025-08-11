#ifndef MCPKG_FS_STD_PATHS_H
#define MCPKG_FS_STD_PATHS_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "container/mcpkg_str_list.h"
#include "fs/mcpkg_fs_error.h"

MCPKG_BEGIN_DECLS

    typedef enum
{
    MCPKG_FS_TMP = 0,     /* Linux: /tmp            | Windows: %TEMP% (GetTempPath) */
    MCPKG_FS_HOME,        /* Linux: $HOME           | Windows: %USERPROFILE% */
    MCPKG_FS_CONFIG,      /* Linux: /etc            | Windows: %PROGRAMDATA% */
    MCPKG_FS_SHARE,       /* Linux: /usr/share      | Windows: %PROGRAMDATA% */
    MCPKG_FS_CACHE,       /* Linux: /var/cache      | Windows: %LOCALAPPDATA% */
    MCPKG_FS_LOCATION_UNKNOWN
} MCPKG_FS_LOCATION;

/* Append default search locations for 'location' into 'out' (like Qt's standardLocations).
 * For now each location contributes exactly one path on each platform.
 */
MCPKG_API MCPKG_FS_ERROR
mcpkg_fs_default_dir(MCPKG_FS_LOCATION location, McPkgStringList *out);

/* Return a malloc'ed writable directory for 'location' in *out (like Qt's writableLocation).
 * Caller must free(*out). On error, *out is set to NULL.
 */
MCPKG_API MCPKG_FS_ERROR
mcpkg_fs_writable_dir(MCPKG_FS_LOCATION location, char **out);

MCPKG_END_DECLS
#endif /* MCPKG_FS_STD_PATHS_H */
