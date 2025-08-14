/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_NET_MODRINTH_CLIENT_H
#define MCPKG_NET_MODRINTH_CLIENT_H

#include "mcpkg_export.h"

#include <stddef.h>
#include <stdint.h>

#include "net/mcpkg_net_client.h"
#include "mcpkg_net_util.h"

#include "container/mcpkg_list.h"

struct McPkgCache;          /* mp: pkg.meta */
struct McPkgNetBuf;

/* Modrinth HTTP client wrapper */
typedef struct McPkgModrinthClient McPkgModrinthClient;

/* config */
typedef struct {
	const char      *base_url;              /* "https://api.modrinth.com" */
	const char      *user_agent;            /* optional */
	const char *const
	*default_headers;     /* optional NULL-terminated "Name: value" */
	long            connect_timeout_ms;     /* <=0 -> default */
	long            operation_timeout_ms;   /* <=0 -> default */
} McPkgModrinthClientCfg;

/* return codes */
#define MCPKG_MODR_NO_ERROR            0
#define MCPKG_MODR_ERR_INVALID         1
#define MCPKG_MODR_ERR_NOMEM           2
#define MCPKG_MODR_ERR_HTTP            3
#define MCPKG_MODR_ERR_JSON            4
#define MCPKG_MODR_ERR_PARSE           5

MCPKG_BEGIN_DECLS

/* lifecycle */
MCPKG_API McPkgModrinthClient *
mcpkg_net_modrinth_client_new(const McPkgModrinthClientCfg *cfg);

MCPKG_API void
mcpkg_net_modrinth_client_free(McPkgModrinthClient *c);

/* rate limit passthrough */
MCPKG_API McPkgNetRateLimit
mcpkg_net_modrinth_get_ratelimit(McPkgModrinthClient *c);

/* ---------- RAW FETCHERS (return raw JSON) ---------- */

/* GET /v2/search?facets=[[...]]&limit=..&offset=.. */
MCPKG_API int
mcpkg_net_modrinth_search_raw(McPkgModrinthClient *c,
                              const char *loader,            /* e.g. "fabric" */
                              const char *mc_version,        /* e.g. "1.21.8" */
                              int limit, int offset,
                              struct McPkgNetBuf *out_body,
                              long *out_http);

/* GET /v2/project/{id|slug}/version?game_versions=[...]&loaders=[...] */
MCPKG_API int
mcpkg_net_modrinth_versions_raw(McPkgModrinthClient *c,
                                const char *id_or_slug,
                                const char *loader,
                                const char *mc_version,
                                struct McPkgNetBuf *out_body,
                                long *out_http);

/* ---------- HIGH-LEVEL: BUILD PKG LIST FROM ONE SEARCH PAGE ---------- */

/*
 * Fetch a search page, then for each hit fetch versions filtered by (loader,mc_version),
 * select the best version (first entry), and materialize a McPkgCache.
 *
 * out_pkgs: list<struct McPkgCache *> (caller owns; free with mcpkg_mp_pkg_meta_free on each elt, then mcpkg_list_free)
 */
MCPKG_API int
mcpkg_net_modrinth_fetch_page_build(McPkgModrinthClient *c,
                                    const char *loader,
                                    const char *mc_version,
                                    int limit, int offset,
                                    struct McPkgList **out_pkgs);

MCPKG_END_DECLS
#endif /* MCPKG_NET_MODRINTH_CLIENT_H */
