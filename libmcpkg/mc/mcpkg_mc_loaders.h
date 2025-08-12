#ifndef MCPKG_MC_LOADERS_H
#define MCPKG_MC_LOADERS_H

#include <stddef.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS

// FLAGS
#define MCPKG_MC_LOADER_F_SUPPORTS_CLIENT   (1u << 0)
#define MCPKG_MC_LOADER_F_SUPPORTS_SERVER   (1u << 1)
#define MCPKG_MC_LOADER_F_HAS_API           (1u << 2)
// MessagePack FLAGS
#define MCPKG_MC_LOADER_MP_TAG        "libmcpkg.mc.loader"
#define MCPKG_MC_LOADER_MP_VERSION    1

// Types
typedef enum MCPKG_MC_LOADERS {
    MCPKG_MC_LOADER_UNKNOWN = 0,
    MCPKG_MC_LOADER_VANILLA,
    MCPKG_MC_LOADER_FORGE,
    MCPKG_MC_LOADER_FABRIC,
    MCPKG_MC_LOADER_QUILT,
    MCPKG_MC_LOADER_PAPER,
    MCPKG_MC_LOADER_PURPUR,
    MCPKG_MC_LOADER_VELOCITY,
    MCPKG_MC_LOADER_END
} MCPKG_MC_LOADERS;

// Loader struct
struct McPkgMcLoaderOps;
struct McPkgMcLoader {
    MCPKG_MC_LOADERS                loader;
    const char                      *name;
    const char                      *base_url;     // optional
    unsigned int                    flags;
    const struct McPkgMcLoaderOps   *ops;
    void                            *priv;
    int                             owns_base_url;
};

// Optional ops for runtime behaviors
struct McPkgMcLoaderOps {
    int  (*is_online)(struct McPkgMcLoader *l);
    void (*destroy)(struct McPkgMcLoader *l);
};

// === API ===
MCPKG_API int  mcpkg_mc_loader_new(struct McPkgMcLoader **outp, MCPKG_MC_LOADERS id);
MCPKG_API void mcpkg_mc_loader_free(struct McPkgMcLoader *l);

MCPKG_API struct McPkgMcLoader mcpkg_mc_loader_make(MCPKG_MC_LOADERS id);
MCPKG_API struct McPkgMcLoader mcpkg_mc_loader_from_string_canon(const char *s);

MCPKG_API const char *mcpkg_mc_loader_to_string(MCPKG_MC_LOADERS id);
MCPKG_API MCPKG_MC_LOADERS mcpkg_mc_loader_from_string(const char *s);

MCPKG_API const struct McPkgMcLoader *mcpkg_mc_loader_table(size_t *count);

MCPKG_API int  mcpkg_mc_loader_is_known(MCPKG_MC_LOADERS id);
MCPKG_API int  mcpkg_mc_loader_requires_network(const struct McPkgMcLoader *l);
MCPKG_API int  mcpkg_mc_loader_is_online(struct McPkgMcLoader *l);

MCPKG_API void mcpkg_mc_loader_set_base_url(struct McPkgMcLoader *l, const char *base_url);
MCPKG_API int  mcpkg_mc_loader_set_base_url_dup(struct McPkgMcLoader *l, const char *base_url);
MCPKG_API void mcpkg_mc_loader_set_online(struct McPkgMcLoader *l, int online);
MCPKG_API void mcpkg_mc_loader_set_ops(struct McPkgMcLoader *l, const struct McPkgMcLoaderOps *ops);

// MessagePack
MCPKG_API int
mcpkg_mc_loader_pack(const struct McPkgMcLoader *l,
                     void **out_buf, size_t *out_len);

MCPKG_API int
mcpkg_mc_loader_unpack(const void *buf, size_t len,
                       struct McPkgMcLoader *out);

MCPKG_END_DECLS
#endif
