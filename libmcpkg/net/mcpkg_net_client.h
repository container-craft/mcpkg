#ifndef MCPKG_NET_CLIENT_H
#define MCPKG_NET_CLIENT_H

#include "mcpkg_export.h"
#include "mcpkg_net_util.h"

#include <stddef.h>

MCPKG_BEGIN_DECLS


struct McPkgNetClient;
typedef struct McPkgNetClient McPkgNetClient;

/* Rate limit snapshot */
typedef struct {
    long		limit;		/* -1 if unknown */
    long		remaining;	/* -1 if unknown */
    long		reset;		/* epoch secs or provider-defined; -1 if unknown */
} McPkgNetRateLimit;

/* Client config */
typedef struct {
    const char	*base_url;		/* required */
    const char	*user_agent;		/* optional */
    const char *const *default_headers;	/* NULL-terminated vector of "Name: value" */
    long		connect_timeout_ms;	/* <=0 -> default */
    long		operation_timeout_ms;	/* <=0 -> default */
} McPkgNetClientCfg;

/* one-time lib init/cleanup */
MCPKG_API int  mcpkg_net_global_init(void);
MCPKG_API void mcpkg_net_global_cleanup(void);

/* lifecycle */
MCPKG_API McPkgNetClient *mcpkg_net_client_new(const McPkgNetClientCfg *cfg);
MCPKG_API void            mcpkg_net_client_free(McPkgNetClient *c);

/* options */
MCPKG_API int mcpkg_net_client_set_timeout(McPkgNetClient *c, long connect_ms, long op_ms);
MCPKG_API int mcpkg_net_client_set_header(McPkgNetClient *c, const char *header_line);
MCPKG_API int mcpkg_net_client_clear_headers(McPkgNetClient *c);
MCPKG_API int mcpkg_net_client_set_user_agent(McPkgNetClient *c, const char *ua);

/* query_kv_pairs: NULL-terminated [key, value, key, value, ... , NULL] */
/* request */
MCPKG_API int mcpkg_net_request(McPkgNetClient *c,
                                const char *method,
                                const char *path,
                                const char *const *query_kv_pairs,
                                const void *body, size_t body_len,
                                struct McPkgNetBuf *out_body,
                                long *out_http_code);

/* helpers */
MCPKG_API int mcpkg_net_get(McPkgNetClient *c,
                            const char *path,
                            const char *const *query_kv_pairs,
                            struct McPkgNetBuf *out_body,
                            long *out_http_code);

MCPKG_API int mcpkg_net_post(McPkgNetClient *c,
                             const char *path,
                             const void *body, size_t body_len,
                             const char *const *query_kv_pairs,
                             struct McPkgNetBuf *out_body,
                             long *out_http_code);

MCPKG_API int mcpkg_net_put(McPkgNetClient *c,
                            const char *path,
                            const void *body, size_t body_len,
                            const char *const *query_kv_pairs,
                            struct McPkgNetBuf *out_body,
                            long *out_http_code);

MCPKG_API int mcpkg_net_delete(McPkgNetClient *c,
                               const char *path,
                               const char *const *query_kv_pairs,
                               struct McPkgNetBuf *out_body,
                               long *out_http_code);

/* last parsed X-RateLimit-* snapshot */
MCPKG_API McPkgNetRateLimit mcpkg_net_get_ratelimit(McPkgNetClient *c);

MCPKG_END_DECLS
#endif /* MCPKG_NET_CLIENT_H */
