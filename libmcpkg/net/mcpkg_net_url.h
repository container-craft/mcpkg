#ifndef MCPKG_NET_URL_H
#define MCPKG_NET_URL_H

#include <stddef.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS

typedef struct McPkgNetUrl McPkgNetUrl;

MCPKG_API int
mcpkg_net_url_new(McPkgNetUrl **out);

MCPKG_API void
mcpkg_net_url_free(McPkgNetUrl *u);

MCPKG_API int
mcpkg_net_url_clear(McPkgNetUrl *u);

MCPKG_API int
mcpkg_net_url_set_url(McPkgNetUrl *u, const char *url);

MCPKG_API int
mcpkg_net_url_is_empty(McPkgNetUrl *u, int *out_is_empty);

MCPKG_API int
mcpkg_net_url_has_query(McPkgNetUrl *u, int *out_has_query);

MCPKG_API int
mcpkg_net_url_has_fragment(McPkgNetUrl *u, int *out_has_fragment);

MCPKG_API int
mcpkg_net_url_scheme(McPkgNetUrl *u, char *buf, size_t buf_sz);

MCPKG_API int
mcpkg_net_url_path(McPkgNetUrl *u, char *buf, size_t buf_sz);

MCPKG_API int
mcpkg_net_url_password(McPkgNetUrl *u, char *buf, size_t buf_sz);

MCPKG_API int
mcpkg_net_url_query(McPkgNetUrl *u, char *buf, size_t buf_sz);

MCPKG_API int
mcpkg_net_url_fragment(McPkgNetUrl *u, char *buf, size_t buf_sz);

MCPKG_API int
mcpkg_net_url_host(McPkgNetUrl *u, char *buf, size_t buf_sz);

MCPKG_API int
mcpkg_net_url_host_ascii(McPkgNetUrl *u, char *buf, size_t buf_sz);

MCPKG_API int
mcpkg_net_url_port(McPkgNetUrl *u, int *out_port);

MCPKG_API int
mcpkg_net_url_set_scheme(McPkgNetUrl *u, const char *scheme);

MCPKG_API int
mcpkg_net_url_set_path(McPkgNetUrl *u, const char *path_utf8);

MCPKG_API int
mcpkg_net_url_set_password(McPkgNetUrl *u, const char *password_utf8);

MCPKG_API int
mcpkg_net_url_set_query(McPkgNetUrl *u, const char *query_utf8);

MCPKG_API int
mcpkg_net_url_set_fragment(McPkgNetUrl *u, const char *fragment_utf8);

MCPKG_API int
mcpkg_net_url_set_host(McPkgNetUrl *u, const char *host_utf8);

MCPKG_API int
mcpkg_net_url_set_port(McPkgNetUrl *u, int port);

// Recompose full URL into caller buffer (absolute if possible)
MCPKG_API int
mcpkg_net_url_to_string(McPkgNetUrl *u, char *buf, size_t buf_sz);

MCPKG_END_DECLS
#endif // MCPKG_NET_URL_H
