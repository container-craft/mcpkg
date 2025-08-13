#include "mcpkg_net_util.h"

// #include <cstdint>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
MCPKG_API const char *
mcpkg_net_strerror(int err)
{
	const char *s = "unknown";
	switch (err) {
		case MCPKG_NET_NO_ERROR:
			s = "ok";
			break;
		case MCPKG_NET_ERR_INVALID:
			s = "invalid";
			break;
		case MCPKG_NET_ERR_SYS:
			s = "syscall/library";
			break;
		case MCPKG_NET_ERR_TIMEOUT:
			s = "timeout";
			break;
		case MCPKG_NET_ERR_DNS:
			s = "dns";
			break;
		case MCPKG_NET_ERR_CONNECT:
			s = "connect";
			break;
		case MCPKG_NET_ERR_HANDSHAKE:
			s = "handshake";
			break;
		case MCPKG_NET_ERR_PROTO:
			s = "protocol";
			break;
		case MCPKG_NET_ERR_CLOSED:
			s = "closed";
			break;
		case MCPKG_NET_ERR_NOMEM:
			s = "nomem";
			break;
		case MCPKG_NET_ERR_RANGE:
			s = "range";
			break;
		case MCPKG_NET_ERR_RATELIMIT:
			s = "ratelimit";
			break;
		default:
			break;
	}
	return s;
}

MCPKG_API int
mcpkg_net_buf_init(struct McPkgNetBuf *b, size_t initial_cap)
{
	int ret = MCPKG_NET_NO_ERROR;

	if (!b)
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR) {
		b->data = NULL;
		b->len = 0;
		b->cap = 0;
		if (initial_cap) {
			b->data = (unsigned char *)malloc(initial_cap);
			if (!b->data) {
				ret = MCPKG_NET_ERR_NOMEM;
			} else {
				b->cap = initial_cap;
			}
		}
	}
	return ret;
}

MCPKG_API int
mcpkg_net_buf_reserve(struct McPkgNetBuf *b, size_t need_cap)
{
	int ret = MCPKG_NET_NO_ERROR;

	if (!b)
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR && b->cap < need_cap) {
		size_t new_cap = b->cap ? b->cap : 64;
		while (new_cap < need_cap) {
			if (new_cap > (SIZE_MAX / 2)) {
				ret = MCPKG_NET_ERR_RANGE;
				break;
			}
			new_cap *= 2;
		}
		if (ret == MCPKG_NET_NO_ERROR) {
			void *p = realloc(b->data, new_cap);
			if (!p) {
				ret = MCPKG_NET_ERR_NOMEM;
			} else {
				b->data = (unsigned char *)p;
				b->cap = new_cap;
			}
		}
	}
	return ret;
}

MCPKG_API int
mcpkg_net_buf_append(struct McPkgNetBuf *b, const void *data, size_t n)
{
	int ret = MCPKG_NET_NO_ERROR;

	if (!b || (!data && n))
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR) {
		size_t need = b->len + n;
		if (need < b->len) /* overflow */
			ret = MCPKG_NET_ERR_RANGE;
	}
	if (ret == MCPKG_NET_NO_ERROR) {
		ret = mcpkg_net_buf_reserve(b, b->len + n);
	}
	if (ret == MCPKG_NET_NO_ERROR && n) {
		memcpy(b->data + b->len, data, n);
		b->len += n;
	}
	return ret;
}

MCPKG_API void
mcpkg_net_buf_reset(struct McPkgNetBuf *b)
{
	if (!b)
		return;
	b->len = 0;
}

MCPKG_API void
mcpkg_net_buf_free(struct McPkgNetBuf *b)
{
	if (!b)
		return;
	free(b->data);
	b->data = NULL;
	b->len = 0;
	b->cap = 0;
}

MCPKG_API int
mcpkg_net_parse_hostport(const char *s,
                         char *host, size_t host_sz,
                         char *port, size_t port_sz)
{
	int ret = MCPKG_NET_NO_ERROR;

	if (!s || !host || !port || host_sz == 0 || port_sz == 0) {
		ret = MCPKG_NET_ERR_INVALID;
	} else if (*s == '[') {
		/* [ipv6]:port */
		const char *end = strchr(s, ']');
		const char *colon = NULL;
		size_t hlen;

		if (!end) {
			ret = MCPKG_NET_ERR_INVALID;
		} else {
			colon = end + 1;
			if (*colon != ':') {
				ret = MCPKG_NET_ERR_INVALID;
			} else {
				colon++;
				hlen = (size_t)(end - (s + 1));
				if (hlen + 1 > host_sz || strlen(colon) + 1 > port_sz) {
					ret = MCPKG_NET_ERR_RANGE;
				} else {
					memcpy(host, s + 1, hlen);
					host[hlen] = '\0';
					memcpy(port, colon, strlen(colon) + 1);
				}
			}
		}
	} else {
		/* host:port (last colon splits; supports IPv6 without brackets if not ambiguous) */
		const char *colon = strrchr(s, ':');
		size_t hlen;

		if (!colon) {
			ret = MCPKG_NET_ERR_INVALID;
		} else {
			hlen = (size_t)(colon - s);
			if (hlen + 1 > host_sz || strlen(colon + 1) + 1 > port_sz) {
				ret = MCPKG_NET_ERR_RANGE;
			} else {
				memcpy(host, s, hlen);
				host[hlen] = '\0';
				memcpy(port, colon + 1, strlen(colon + 1) + 1);
			}
		}
	}

	return ret;
}
