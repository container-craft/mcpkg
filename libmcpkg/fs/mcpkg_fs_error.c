#include "mcpkg_fs_error.h"

const char *mcpkg_fs_strerror(MCPKG_FS_ERROR e)
{
    switch (e) {
    case MCPKG_FS_OK:             return "ok";
    case MCPKG_FS_ERR_NULL_PARAM: return "null parameter";
    case MCPKG_FS_ERR_NOT_FOUND:  return "not found";
    case MCPKG_FS_ERR_EXISTS:     return "already exists";
    case MCPKG_FS_ERR_PERM:       return "permission denied";
    case MCPKG_FS_ERR_IO:         return "io error";
    case MCPKG_FS_ERR_NOSPC:      return "no space";
    case MCPKG_FS_ERR_RANGE:      return "out of range";
    case MCPKG_FS_ERR_OVERFLOW:   return "overflow";
    case MCPKG_FS_ERR_OOM:        return "out of memory";
    case MCPKG_FS_ERR_UNSUPPORTED:return "unsupported";
    case MCPKG_FS_ERR_OTHER:      return "other";
    }
    return "unknown";
}
