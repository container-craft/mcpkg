#ifndef MCPKG_FS_FILE_H
#define MCPKG_FS_FILE_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "fs/mcpkg_fs_error.h"

MCPKG_BEGIN_DECLS

/* create/truncate file (parent dirs not auto-created here). */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_touch(const char *path);

/* unlink file if present (file or empty dir). */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_unlink(const char *path);

/* copy file bytes src→dst. if overwrite==0 and dst exists, ERR_EXISTS. */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_cp_file(const char *src, const char *dst,
                int overwrite);

/* read whole file into malloc'd buffer; binary-safe. */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_read_all(const char *path,
                unsigned char **buf_out,
                size_t *size_out);

/* write whole buffer to path (overwrite: 0=O_EXCL, 1=truncate/replace). */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_write_all(const char *path,
                const void *data, size_t size,
                int overwrite);

/* Zstd: compress buffer → file (level typically 1..22). */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_write_zstd(const char *path,
                const void *data, size_t size,
                int level);

/* Zstd: read+decompress entire file → malloc'd buffer. */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_read_zstd(const char *path,
                unsigned char **buf_out,
                size_t *size_out);

/* Create/replace symlink (POSIX). On Windows returns UNSUPPORTED. */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_ln_sf(const char *target,
                                        const char *link_path,
                                        int overwrite);

/* Read symlink target into caller buffer. On POSIX:
 * - buf_size includes space for optional NUL we add if room.
 * - out_len gets set to number of bytes (without NUL).
 * Returns ERR_RANGE if buffer too small.
 * On Windows returns UNSUPPORTED (unless you add a reparse-point impl).
 */
MCPKG_API MCPKG_FS_ERROR mcpkg_fs_link_target(const char *link_path,
                char *buffer, size_t buf_size,
                size_t *out_len);

/* Return 1 if an existing regular file, 0 if not, <0 on error. */
MCPKG_API int mcpkg_fs_file_exists(const char *path);

MCPKG_END_DECLS
#endif /* MCPKG_FS_FILE_H */
