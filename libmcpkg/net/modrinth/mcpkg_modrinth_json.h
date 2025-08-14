/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_MODRINTH_JSON_H
#define MCPKG_MODRINTH_JSON_H

#include "mcpkg_export.h"
#include "container/mcpkg_list.h"
#include "container/mcpkg_str_list.h"
#include "mp/mcpkg_mp_pkg_meta.h"

#include <stdint.h>
#include <stddef.h>

MCPKG_BEGIN_DECLS

// return codes
#define MCPKG_MODRINTH_JSON_NO_ERROR          0
#define MCPKG_MODRINTH_JSON_ERR_INVALID       1
#define MCPKG_MODRINTH_JSON_ERR_NOMEM         2
#define MCPKG_MODRINTH_JSON_ERR_PARSE         3

// flags describing what we found when building McPkgCache
#define MCPKG_MODRINTH_F_HAS_SHA512           (1u << 0)
#define MCPKG_MODRINTH_F_HAS_SHA1             (1u << 1)
#define MCPKG_MODRINTH_F_HAS_INCOMPAT_DEPS    (1u << 2)
#define MCPKG_MODRINTH_F_HAS_EMBEDDED_DEPS    (1u << 3)

// minimal hit info we need from /v2/search hits[] to enrich meta
struct McPkgModrinthHit {
	char                    *project_id;    // stable id
	char                    *slug;          // slug
	char                    *title;         // title
	char                    *description;   // short text
	char                    *license_id;    // SPDX or LicenseRef
	char                    *icon_url;      // optional
	int32_t                 client;         // UNKNOWN=-1, NO=0, YES=1
	int32_t                 server;         // UNKNOWN=-1, NO=0, YES=1
	struct McPkgStringList  *categories;    // optional
};

// free one McPkgModrinthHit
MCPKG_API void mcpkg_modrinth_hit_free(struct McPkgModrinthHit *h);

// parse /v2/search page into a list of McPkgModrinthHit* and total_hits
MCPKG_API int mcpkg_modrinth_parse_search_hits_detailed(const char *json,
                size_t json_len,
                struct McPkgList **out_hits,   // of McPkgModrinthHit*
                uint64_t *out_total_hits);

// convenience: parse only identifiers (project_id or slug) into a list of char*
MCPKG_API int mcpkg_modrinth_parse_search_hit_ids(const char *json,
                size_t json_len,
                struct McPkgList **out_ids,         // of char*
                uint64_t *out_total_hits);

// choose best version index from /v2/project/{id|slug}/version array
// applies dynamic filters: want_loader, want_game_version, featured_only
MCPKG_API int mcpkg_modrinth_choose_version_idx(const char *versions_json,
                size_t versions_len,
                const char *want_loader,
                const char *want_game_version,
                int featured_only,                     // 0/1
                int *out_idx);                         // -1 if none

// build McPkgCache from one chosen version element and an optional hit
// - hit can be NULL; fields from hit override only when present
// - provider should be "modrinth" (used for origin)
// outputs:
//   out_cache: fully populated (id, slug, version, title, description, license_id,
//              loaders, file(digests/url/size/name), depends[], client/server, origin)
//   out_flags: MCPKG_MODRINTH_F_* bits
MCPKG_API int mcpkg_modrinth_build_pkg_meta(const struct McPkgModrinthHit *hit,
                const char *versions_json,
                size_t versions_len,
                int ver_idx,
                const char *provider,
                struct McPkgCache **out_cache,
                uint32_t *out_flags);

MCPKG_END_DECLS
#endif /* MCPKG_MODRINTH_JSON_H */
