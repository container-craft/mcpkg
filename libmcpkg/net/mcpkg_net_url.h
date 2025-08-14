/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_NET_URL_H
#define MCPKG_NET_URL_H

#include "mcpkg_export.h"
#include "mcpkg_net_util.h"

#include <stddef.h>

MCPKG_BEGIN_DECLS

struct McPkgNetUrl;
typedef struct McPkgNetUrl McPkgNetUrl;

/* lifecycle */
MCPKG_API McPkgNetUrl *mcpkg_net_url_new(void);
MCPKG_API void         mcpkg_net_url_free(McPkgNetUrl *u);
MCPKG_API McPkgNetUrl *mcpkg_net_url_clone(McPkgNetUrl
                *u); /* deep clone, NULL on OOM */

/* whole-URL parse / clear */
MCPKG_API int mcpkg_net_url_parse(McPkgNetUrl *u, const char *url_utf8);
MCPKG_API int mcpkg_net_url_clear(McPkgNetUrl *u);

/* predicates/getters (non-allocating) */
MCPKG_API int mcpkg_net_url_is_empty(McPkgNetUrl *u, int *out_is_empty);
MCPKG_API int mcpkg_net_url_has_query(McPkgNetUrl *u, int *out_has_query);
MCPKG_API int mcpkg_net_url_has_fragment(McPkgNetUrl *u, int *out_has_fragment);

MCPKG_API int mcpkg_net_url_scheme(McPkgNetUrl *u, char *buf, size_t buf_sz);
MCPKG_API int mcpkg_net_url_host(McPkgNetUrl *u, char *buf, size_t buf_sz);
MCPKG_API int mcpkg_net_url_host_ascii(McPkgNetUrl *u, char *buf,
                                       size_t buf_sz);
MCPKG_API int mcpkg_net_url_port(McPkgNetUrl *u, int *out_port);
MCPKG_API int mcpkg_net_url_path(McPkgNetUrl *u, char *buf, size_t buf_sz);
MCPKG_API int mcpkg_net_url_query(McPkgNetUrl *u, char *buf, size_t buf_sz);
MCPKG_API int mcpkg_net_url_fragment(McPkgNetUrl *u, char *buf, size_t buf_sz);

/* setters (auto-encode where appropriate) */
MCPKG_API int mcpkg_net_url_set_scheme(McPkgNetUrl *u,
                                       const char *scheme_ascii);
MCPKG_API int mcpkg_net_url_set_host(McPkgNetUrl *u, const char *host_utf8);
MCPKG_API int mcpkg_net_url_set_port(McPkgNetUrl *u, int port); /* 0 clears */
MCPKG_API int mcpkg_net_url_set_path(McPkgNetUrl *u, const char *path_utf8);
MCPKG_API int mcpkg_net_url_set_query(McPkgNetUrl *u,
                                      const char *query_no_qmark);
MCPKG_API int mcpkg_net_url_add_query(McPkgNetUrl *u, const char *key_utf8,
                                      const char *val_utf8);
MCPKG_API int mcpkg_net_url_set_fragment(McPkgNetUrl *u,
                const char *fragment_no_hash);
MCPKG_API int mcpkg_net_url_set_password(McPkgNetUrl *u,
                const char *password_utf8);

/* to-string */
MCPKG_API int mcpkg_net_url_to_string_buf(McPkgNetUrl *u, char *buf,
                size_t buf_sz);
/* Allocates a fresh copy (malloc). Caller must free(*out). */
MCPKG_API int mcpkg_net_url_to_string(McPkgNetUrl *u, char **out);

MCPKG_API int mcpkg_net_url_vaild_schema(const char *s);


MCPKG_API int mcpkg_net_url_is_abs(const char *p);


MCPKG_END_DECLS
#endif /* MCPKG_NET_URL_H */
