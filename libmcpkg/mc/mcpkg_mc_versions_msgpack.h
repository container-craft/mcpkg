#ifndef MCPKG_MC_VERSIONS_MSGPACK_H
#define MCPKG_MC_VERSIONS_MSGPACK_H

#include "mcpkg_export.h"
#include "mc/mcpkg_mc_versions.h"

MCPKG_BEGIN_DECLS

#define MCPKG_MC_VERSION_FAM_MP_TAG      "libmcpkg.mc.version_family"
#define MCPKG_MC_VERSION_FAM_MP_VERSION  1

// Serialize a version family to a newly allocated MessagePack buffer.
MCPKG_API int
mcpkg_mc_version_family_pack(const struct McPkgMCVersion *v,
                             void **out_buf, size_t *out_len);


// Deserialize
MCPKG_API int
mcpkg_mc_version_family_unpack(const void *buf, size_t len,
                               struct McPkgMCVersion *out);

MCPKG_END_DECLS

#endif /* MCPKG_MC_VERSIONS_MSGPACK_H */
