/* SPDX-License-Identifier: MIT */
/* mc_net_client.h — high-level HTTP client built on libcurl-openssl.
 * Uses net/mcpkg_net_url for robust URL handling and mcpkg_net_util for buffers/errors.
 */

#ifndef MC_NET_CLIENT_H
#define MC_NET_CLIENT_H

#include <stddef.h>

#include "mcpkg_export.h"
#include "net/mcpkg_net_util.h"

MCPKG_BEGIN_DECLS

typedef struct McPkgNetClient McPkgNetClient;

/* HTTP methods */
typedef enum MCPKG_NET_METHOD {
	MCPKG_NET_METHOD_GET = 0,
	MCPKG_NET_METHOD_POST,
	MCPKG_NET_METHOD_PUT,
	MCPKG_NET_METHOD_DELETE
} MCPKG_NET_METHOD;

/* Rate limit snapshot (from last response; -1 if unknown) */
typedef struct McPkgNetRateLimit {
	long limit;
	long remaining;
	long reset;
} McPkgNetRateLimit;

/* Global libcurl init/cleanup (call once per process) */
MCPKG_API int  mcpkg_net_global_init(void);
MCPKG_API void mcpkg_net_global_cleanup(void);

/* Create/destroy client
 * base_url: e.g. "https://api.modrinth.com/v2"
 * user_agent: required (e.g., "github_user/project/1.0 (contact)")
 */
MCPKG_API int  mcpkg_net_client_new(McPkgNetClient **out,
                                    const char *base_url,
                                    const char *user_agent);
MCPKG_API void mcpkg_net_client_free(McPkgNetClient *c);

/* Configuration */
MCPKG_API int mcpkg_net_set_header(McPkgNetClient *c, const char *header_line);
MCPKG_API int mcpkg_net_set_timeout(McPkgNetClient *c, long connect_ms,
                                    long total_ms);
MCPKG_API int mcpkg_net_set_offline(McPkgNetClient *c, int on_off);
MCPKG_API int mcpkg_net_set_offline_root(McPkgNetClient *c, const char *dir);
/* Placeholder for future pooling; currently single handle only. */
MCPKG_API int mcpkg_net_set_pool_size(McPkgNetClient *c, int n);

/* One‑shot blocking request
 * query_kv_pairs: NULL-terminated ["k","v","k","v",NULL] or NULL
 * body/body_len: for POST/PUT (may be NULL/0)
 * out_body: caller-initialized buffer (len reset on entry)
 * out_http_code: optional
 */
MCPKG_API int mcpkg_net_request(McPkgNetClient *c,
                                MCPKG_NET_METHOD method,
                                const char *path,              /* appended to base URL */
                                const char
                                *query_kv_pairs[],  /* NULL-terminated ["k","v","k","v",NULL] or NULL */
                                const void *body, size_t body_len, /* for POST/PUT; may be NULL */
                                struct McPkgNetBuf *out_body,  /* caller-initialized buf (len reset on entry) */
                                long *out_http_code);

/* Rate limit from last response */
MCPKG_API int mcpkg_net_get_ratelimit(McPkgNetClient *c,
                                      McPkgNetRateLimit *out);

MCPKG_END_DECLS
#endif /* MC_NET_CLIENT_H */
