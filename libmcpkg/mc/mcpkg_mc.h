#ifndef MCPKG_MC_H
#define MCPKG_MC_H

#include <stddef.h>
#include "mcpkg_export.h"

// MC submodules
#include "mc/mcpkg_mc_util.h"
#include "mc/mcpkg_mc_loaders.h"
#include "mc/mcpkg_mc_loaders_msgpack.h"
#include "mc/mcpkg_mc_providers.h"
#include "mc/mcpkg_mc_versions.h"
#include "mc/mcpkg_mc_versions_msgpack.h"

// Container submodules
#include "container/mcpkg_list.h"
#include "container/mcpkg_str_list.h"

MCPKG_BEGIN_DECLS

// Env defaults (kept for CLI convenience will be going away)
#define MCPKG_ENV_MC_VERSION    "MC_VERSION"
#define MCPKG_MC_DEFAULT_VERSION "1.21.8"

struct McPkgMc {
    struct McPkgMCVersion       *current_version;       // owned
    struct McPkgMcProvider      *current_provider;      // owned
    struct McPkgMcLoader        *current_loader;        // owned
    McPkgList                   *versions;
    McPkgList                   *providers;
    McPkgList                   *loaders;
    unsigned int			flags;
};


MCPKG_API int  mcpkg_mc_new(struct McPkgMc **out);
MCPKG_API void mcpkg_mc_free(struct McPkgMc *mc);

// singleton (lazy init)
MCPKG_API int               mcpkg_mc_global_init(void);
MCPKG_API struct McPkgMc    *mcpkg_mc_global(void);     // NULL until init succeeds
MCPKG_API void              mcpkg_mc_global_shutdown(void);

// Seeding helpers (populate known providers/loaders and version families)
// Safe to call multiple times; duplicates are ignored by identity.
MCPKG_API int mcpkg_mc_seed_providers(struct McPkgMc *mc);
MCPKG_API int mcpkg_mc_seed_loaders(struct McPkgMc *mc);

// Version families are often large; seeding is split to keep call sites flexible.

// just latest family/version
MCPKG_API int
mcpkg_mc_seed_versions_minimal(struct McPkgMc *mc);

// full built-in canon set
MCPKG_API int
mcpkg_mc_seed_versions_all(struct McPkgMc *mc);

// Add/own entries (mc takes ownership on success)
MCPKG_API int
mcpkg_mc_add_provider(struct McPkgMc *mc,
                      struct McPkgMcProvider *p);

MCPKG_API int
mcpkg_mc_add_loader(struct McPkgMc *mc,
                    struct McPkgMcLoader *l);

MCPKG_API int
mcpkg_mc_add_version_family(struct McPkgMc *mc,
                            struct McPkgMCVersion *vf);

// Find helpers (borrowed pointers; do not free !!)
MCPKG_API struct McPkgMcProvider*
mcpkg_mc_find_provider_id(struct McPkgMc *mc,
                          MCPKG_MC_PROVIDERS id);

MCPKG_API struct McPkgMcProvider*
mcpkg_mc_find_provider_name(struct McPkgMc *mc, const char *name);

MCPKG_API struct McPkgMcLoader*
mcpkg_mc_find_loader_id(struct McPkgMc *mc, MCPKG_MC_LOADERS id);

MCPKG_API struct McPkgMcLoader*
mcpkg_mc_find_loader_name(struct McPkgMc *mc, const char *name);

MCPKG_API struct McPkgMCVersion*
mcpkg_mc_find_family_code(struct McPkgMc *mc, MCPKG_MC_CODE_NAME code);

MCPKG_API struct McPkgMCVersion*
mcpkg_mc_find_family_slug(struct McPkgMc *mc, const char *codename_slug);

// Current selection setters (by pointer or enum/string)
MCPKG_API int
mcpkg_mc_set_current_provider(struct McPkgMc *mc,
                              struct McPkgMcProvider *p);

MCPKG_API int
mcpkg_mc_set_current_provider_id(struct McPkgMc *mc,
                                 MCPKG_MC_PROVIDERS id);

MCPKG_API int
mcpkg_mc_set_current_loader(struct McPkgMc *mc,
                            struct McPkgMcLoader *l);
MCPKG_API int
mcpkg_mc_set_current_loader_id(struct McPkgMc *mc,
                               MCPKG_MC_LOADERS id);

MCPKG_API int mcpkg_mc_set_current_family(struct McPkgMc *mc, struct McPkgMCVersion *vf);
MCPKG_API int mcpkg_mc_set_current_family_code(struct McPkgMc *mc, MCPKG_MC_CODE_NAME code);

// Convenience: latest version resolution
// Returns >0 and sets *out_version on success; 0 if unknown; <0 on error.
MCPKG_API int mcpkg_mc_latest_for_codename(struct McPkgMc *mc,
                                           MCPKG_MC_CODE_NAME code,
                                           const char **out_version);

// Convenience: map a version string to codename using current registry
MCPKG_API MCPKG_MC_CODE_NAME
mcpkg_mc_codename_from_version_in(struct McPkgMc *mc, const char *mc_version);

// Serialize selections (optional; keeps module packers separate)
// These only pack the current selections’ identity, not entire registries.
MCPKG_API int mcpkg_mc_pack_current_provider(const struct McPkgMc *mc,
                                             void **out_buf, size_t *out_len);
MCPKG_API int mcpkg_mc_unpack_current_provider(struct McPkgMc *mc,
                                               const void *buf, size_t len);

MCPKG_API int mcpkg_mc_pack_current_loader(const struct McPkgMc *mc,
                                           void **out_buf, size_t *out_len);
MCPKG_API int mcpkg_mc_unpack_current_loader(struct McPkgMc *mc,
                                             const void *buf, size_t len);

MCPKG_API int mcpkg_mc_pack_current_family(const struct McPkgMc *mc,
                                           void **out_buf, size_t *out_len);
MCPKG_API int mcpkg_mc_unpack_current_family(struct McPkgMc *mc,
                                             const void *buf, size_t len);

// Future: pack/unpack whole registries if needed (not required right now)

MCPKG_END_DECLS
#endif /* MCPKG_MC_H */
