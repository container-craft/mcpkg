#ifndef MCPKG_NET_UTIL_H
#define MCPKG_NET_UTIL_H

#include <curl/curl.h>
#include <stddef.h>
#include "mcpkg_export.h"
#include "mcpkg_fs_error.h"

MCPKG_BEGIN_DECLS
typedef enum MCPKG_NET_ERROR {
	MCPKG_NET_NO_ERROR        = 0,
	MCPKG_NET_ERR_INVALID     = 1,
	MCPKG_NET_ERR_SYS         = 2,
	MCPKG_NET_ERR_TIMEOUT     = 3,
	MCPKG_NET_ERR_DNS         = 4,
	MCPKG_NET_ERR_CONNECT     = 5,
	MCPKG_NET_ERR_HANDSHAKE   = 6,
	MCPKG_NET_ERR_PROTO       = 7,
	MCPKG_NET_ERR_CLOSED      = 8,
	MCPKG_NET_ERR_NOMEM       = 9,
	MCPKG_NET_ERR_RANGE       = 10,
	MCPKG_NET_ERR_RATELIMIT   = 11,
	MCPKG_NET_ERR_IO          = 12,
	MCPKG_NET_ERR_TLS         = 14,
	MCPKG_NET_ERR_OTHER       = 200
} MCPKG_NET_ERROR;

/* Return a stable, static string for our error codes. */
MCPKG_API const char *mcpkg_net_strerror(int err);

/* ---- Small dynamic buffer (grow-only) ---- */
struct McPkgNetBuf {
	unsigned char *data; // own
	size_t len;
	size_t cap;          // alloc
};


static int mcpkg_net_utils_fs_err_to_net_err(int fs_err)
{
	switch (fs_err) {
		case MCPKG_FS_OK:
			return MCPKG_NET_NO_ERROR;
		case MCPKG_FS_ERR_OOM:
			return MCPKG_NET_ERR_NOMEM;
		case MCPKG_FS_ERR_NOSPC:
			return MCPKG_NET_ERR_IO;
		case MCPKG_FS_ERR_EXISTS:
			return MCPKG_NET_ERR_OTHER;
		case MCPKG_FS_ERR_NOT_FOUND:
			return MCPKG_NET_ERR_OTHER;
		case MCPKG_FS_ERR_RANGE:
			return MCPKG_NET_ERR_OTHER;
		default:
			return MCPKG_NET_ERR_IO;
	}
}

// static int map_fs_to_net(int fs_err)
// {
//     switch (fs_err) {
//         case MCPKG_FS_OK:
//             return MCPKG_NET_NO_ERROR;
//         case MCPKG_FS_ERR_OOM:
//             return MCPKG_NET_ERR_NOMEM;
//         case MCPKG_FS_ERR_NOSPC:
//             return MCPKG_NET_ERR_IO;
//         case MCPKG_FS_ERR_EXISTS:
//             return MCPKG_NET_ERR_OTHER;
//         case MCPKG_FS_ERR_NOT_FOUND:
//             return MCPKG_NET_ERR_OTHER;
//         case MCPKG_FS_ERR_RANGE:
//             return MCPKG_NET_ERR_OTHER;
//         default:
//             return MCPKG_NET_ERR_IO;
//     }
// }
MCPKG_API int mcpkg_net_curl_to_net_error(CURLcode cc);

MCPKG_API int  mcpkg_net_buf_init(struct McPkgNetBuf *b, size_t initial_cap);
MCPKG_API int  mcpkg_net_buf_reserve(struct McPkgNetBuf *b, size_t need_cap);
MCPKG_API int  mcpkg_net_buf_append(struct McPkgNetBuf *b, const void *data,
                                    size_t n);
MCPKG_API void mcpkg_net_buf_reset(struct McPkgNetBuf
                                   *b); /* len = 0, keep cap */
MCPKG_API void mcpkg_net_buf_free(struct McPkgNetBuf *b);

// Parse "host:port" (handles IPv6 [::1]:443 form). Writes NUL-terminated outputs
MCPKG_API int mcpkg_net_parse_hostport(const char *s,
                                       char *host, size_t host_sz,
                                       char *port, size_t port_sz);

MCPKG_END_DECLS
#endif // MCPKG_NET_UTIL_H
