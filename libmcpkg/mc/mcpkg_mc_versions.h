#ifndef MCPKG_MC_VERSIONS_H
#define MCPKG_MC_VERSIONS_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "container/mcpkg_str_list.h"

MCPKG_BEGIN_DECLS

// MessagePack schema tag/version for a version family
#define MCPKG_MC_VERSION_FAM_MP_TAG     "libmcpkg.mc.version_family"
#define MCPKG_MC_VERSION_FAM_MP_VERSION 1

// Stable codename IDs for storage/wire. Keep numeric values stable.
typedef enum {
	MCPKG_MC_CODE_NAME_UNKNOWN = 0,
	MCPKG_MC_CODE_NAME_TRICKY_TRIALS,
	MCPKG_MC_CODE_NAME_TRAILS_AND_TALES,
	MCPKG_MC_CODE_NAME_THE_WILD,
	MCPKG_MC_CODE_NAME_CAVES_AND_CLIFFS_TWO,
	MCPKG_MC_CODE_NAME_CAVES_AND_CLIFFS_ONE,
	MCPKG_MC_CODE_NAME_NETHER_UPDATE,
	MCPKG_MC_CODE_NAME_BUZZY_BEES,
	MCPKG_MC_CODE_NAME_VILLAGE_AND_PILLAGE,
	MCPKG_MC_CODE_NAME_AQUATIC,
	MCPKG_MC_CODE_NAME_WORLD_OF_COLOR,
	MCPKG_MC_CODE_NAME_EXPLORATION,
	MCPKG_MC_CODE_NAME_FROSTBURN,
	MCPKG_MC_CODE_NAME_COMBAT,
	MCPKG_MC_CODE_NAME_BOUNTIFUL,
	MCPKG_MC_CODE_NAME_CHANGED_THE_WORLD,
	MCPKG_MC_CODE_NAME_HORSE,
	MCPKG_MC_CODE_NAME_REDSTONE,
	MCPKG_MC_CODE_NAME_PRETTY_SCARY,
	MCPKG_MC_CODE_NAME_VILLAGER_TRADING,
	MCPKG_MC_CODE_NAME_FAITHFUL,
	MCPKG_MC_CODE_NAME_SPAWN_EGG,
	MCPKG_MC_CODE_NAME_ADVENTURE,
} MCPKG_MC_CODE_NAME;

// One “version family” (codename + list of versions)
struct McPkgMCVersion {
	MCPKG_MC_CODE_NAME	codename;   // enum ID
	int                  snapshot;   // 0 (release), >0 (snapshot/preview)
	McPkgStringList		*versions;  // newest -> oldest; may be NULL/empty
};

// ---- Constructors / destructors ----
MCPKG_API int  mcpkg_mc_version_family_new(struct McPkgMCVersion **out,
                MCPKG_MC_CODE_NAME code);
MCPKG_API void mcpkg_mc_version_family_free(struct McPkgMCVersion *v);

// Make a by-value family with empty list.
MCPKG_API struct McPkgMCVersion
mcpkg_mc_version_family_make(MCPKG_MC_CODE_NAME code);

// ---- Mapping helpers ----
MCPKG_API const char *mcpkg_mc_codename_to_string(MCPKG_MC_CODE_NAME code);
MCPKG_API MCPKG_MC_CODE_NAME mcpkg_mc_codename_from_string(const char *s);

// ---- Queries ----
// Return >0 if non-empty and set *out_latest to versions[0]; 0 if none; <0 on error.
MCPKG_API int mcpkg_mc_version_family_latest(const struct McPkgMCVersion *v,
                const char **out_latest);

// Find a codename by version string among an array of families.
// Returns enum (>0) if found, MCPKG_MC_CODE_NAME_UNKNOWN if not.
MCPKG_API MCPKG_MC_CODE_NAME
mcpkg_mc_codename_from_version(const struct McPkgMCVersion *const *families,
                               size_t nfam, const char *mc_version);

// ---- MessagePack (de)serialization for one family ----
MCPKG_API int mcpkg_mc_version_family_pack(const struct McPkgMCVersion *v,
                void **out_buf, size_t *out_len);
MCPKG_API int mcpkg_mc_version_family_unpack(const void *buf, size_t len,
                struct McPkgMCVersion *out);

MCPKG_END_DECLS
#endif // MCPKG_MC_VERSIONS_H
