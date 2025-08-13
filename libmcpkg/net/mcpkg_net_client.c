/* SPDX-License-Identifier: MIT */
/* mc_net_client.c — blocking HTTP client built on libcurl-openssl.
 * - Lazy-inits a single CURL easy handle and reuses it.
 * - URL composition/parsing via net/mcpkg_net_url.{h,c} (CURLU).
 * - Returns raw bytes in McPkgNetBuf; providers parse as needed.
 * - Optional offline mode loads files from offline_root.
 * - Tracks X-RateLimit-* headers.
 */

#include "mcpkg_net_client.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <curl/curl.h>

#include "net/mcpkg_net_util.h"
#include "net/mcpkg_net_url.h"


struct McPkgNetClient {
	char *base_url;
	char *user_agent;

	CURL *curl;
	struct curl_slist *headers;

	long connect_timeout_ms;
	long total_timeout_ms;

	int offline;         /* 0/1 */
	char *offline_root;  /* directory for offline JSON/bytes */

	long rl_limit;
	long rl_remaining;
	long rl_reset;

	long last_http_code;
};

/* -------- small utils -------- */

static char *dup_cstr(const char *s)
{
	char *p;
	size_t n;
	if (!s) return NULL;
	n = strlen(s) + 1;
	p = (char *)malloc(n);
	if (p) memcpy(p, s, n);
	return p;
}

static size_t write_body_cb(char *ptr, size_t size, size_t nmemb,
                            void *userdata)
{
	size_t total = size * nmemb;
	struct McPkgNetBuf *b = (struct McPkgNetBuf *)userdata;
	size_t need = b->len + total;

	if (need + 1 < b->len)
		return 0; /* overflow guard */

	if (need + 1 > b->cap) {
		size_t newcap = b->cap ? b->cap * 2 : 4096;
		while (newcap < need + 1)
			newcap *= 2;
		void *q = realloc(b->data, newcap);
		if (!q)
			return 0; /* abort transfer */
		b->data = (unsigned char *)q;
		b->cap = newcap;
	}
	memcpy(b->data + b->len, ptr, total);
	b->len += total;
	b->data[b->len] = 0;
	return total;
}

static size_t write_hdr_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t total = size * nmemb;
	struct McPkgNetClient *c = (struct McPkgNetClient *)userdata;
	const char *p = ptr;
	const char *end = ptr + total;

	if (total == 0) return 0;

	/* key: value\r\n */
	const char *colon = memchr(p, ':', (size_t)(end - p));
	if (!colon) return total;

	size_t key_len = (size_t)(colon - p);
	const char *v = colon + 1;
	while (v < end && (*v == ' ' || *v == '\t')) v++;
	const char *vend = end;
	while (vend > v && (vend[-1] == '\r' || vend[-1] == '\n')) vend--;

	char numbuf[64];
	size_t vlen = (size_t)(vend - v);
	if (vlen >= sizeof(numbuf)) vlen = sizeof(numbuf) - 1;
	memcpy(numbuf, v, vlen);
	numbuf[vlen] = '\0';

#define HKEY_EQ(lit) (key_len == sizeof(lit)-1 && strncasecmp(p, lit, key_len) == 0)
	if (HKEY_EQ("X-RateLimit-Limit")) {
		c->rl_limit = strtol(numbuf, NULL, 10);
	} else if (HKEY_EQ("X-RateLimit-Remaining")) {
		c->rl_remaining = strtol(numbuf, NULL, 10);
	} else if (HKEY_EQ("X-RateLimit-Reset")) {
		c->rl_reset = strtol(numbuf, NULL, 10);
	}
#undef HKEY_EQ
	return total;
}

static int ensure_curl(struct McPkgNetClient *c)
{
	int ret = MCPKG_NET_NO_ERROR;

	if (!c)
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR && !c->curl) {
		CURL *h = curl_easy_init();
		if (!h) {
			ret = MCPKG_NET_ERR_SYS;
		} else {
			curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1L);
			curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 2L);
			curl_easy_setopt(h, CURLOPT_USERAGENT,
			                 c->user_agent ? c->user_agent : "mcpkg/unknown");
			curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT_MS,
			                 c->connect_timeout_ms > 0 ? c->connect_timeout_ms : 10000L);
			curl_easy_setopt(h, CURLOPT_TIMEOUT_MS,
			                 c->total_timeout_ms > 0 ? c->total_timeout_ms : 30000L);
			curl_easy_setopt(h, CURLOPT_HEADERFUNCTION, write_hdr_cb);
			curl_easy_setopt(h, CURLOPT_HEADERDATA, c);

			/* Default header: Accept: application/json */
			c->headers = curl_slist_append(c->headers, "Accept: application/json");
			if (!c->headers) {
				curl_easy_cleanup(h);
				ret = MCPKG_NET_ERR_NOMEM;
			} else {
				curl_easy_setopt(h, CURLOPT_HTTPHEADER, c->headers);
				c->curl = h;
			}
		}
	}
	return ret;
}

/* Build full URL using mcpkg_net_url: base_url + path + query (kv pairs).
 * Returns malloc'd string in *out; caller frees.
 */
static int build_full_url(const char *base_url,
                          const char *path,
                          const char *kv_pairs[], /* NULL-terminated ["k","v",...,NULL] or NULL */
                          char **out)
{
	int ret = MCPKG_NET_NO_ERROR;
	McPkgNetUrl *u = NULL;
	char scheme[16], host[512], base_path[1024];
	char new_path[2048];
	char query[4096];
	int has_query = 0;
	size_t i;
	size_t qlen = 0;

	if (!base_url || !path || !out) {
		ret = MCPKG_NET_ERR_INVALID;
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		ret = mcpkg_net_url_new(&u);
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		ret = mcpkg_net_url_set_url(u, base_url);
	}

	/* Get existing base path so we can join with incoming path */
	if (ret == MCPKG_NET_NO_ERROR) {
		ret = mcpkg_net_url_scheme(u, scheme, sizeof(scheme));
		if (ret == MCPKG_NET_NO_ERROR && !scheme[0]) {
			/* If no scheme, base URL invalid */
			ret = MCPKG_NET_ERR_PROTO;
		}
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		ret = mcpkg_net_url_host(u, host, sizeof(host));
		if (ret == MCPKG_NET_NO_ERROR && !host[0]) {
			ret = MCPKG_NET_ERR_PROTO;
		}
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		ret = mcpkg_net_url_path(u, base_path, sizeof(base_path));
		if (ret == MCPKG_NET_NO_ERROR && !base_path[0]) {
			base_path[0] = '/';
			base_path[1] = '\0';
		}
	}

	/* Join base_path and path with a single slash */
	if (ret == MCPKG_NET_NO_ERROR) {
		const char *a = base_path;
		const char *b = path;
		size_t la = strlen(a);
		size_t lb = strlen(b);

		if (la == 0) {
			snprintf(new_path, sizeof(new_path), "/%s", b[0] == '/' ? (b + 1) : b);
		} else {
			int a_slash_end = (a[la - 1] == '/');
			int b_slash_start = (lb > 0 && b[0] == '/');
			if (a_slash_end && b_slash_start) {
				snprintf(new_path, sizeof(new_path), "%.*s%s", (int)(la - 1), a, b);
			} else if (!a_slash_end && !b_slash_start) {
				snprintf(new_path, sizeof(new_path), "%s/%s", a, b);
			} else {
				snprintf(new_path, sizeof(new_path), "%s%s", a, b);
			}
		}
	}

	/* Apply new path */
	if (ret == MCPKG_NET_NO_ERROR) {
		ret = mcpkg_net_url_set_path(u, new_path);
	}

	/* Build query string from kv_pairs */
	if (ret == MCPKG_NET_NO_ERROR) {
		if (kv_pairs && kv_pairs[0]) {
			query[0] = '\0';
			qlen = 0;
			for (i = 0; kv_pairs[i]; i += 2) {
				const char *k = kv_pairs[i];
				const char *v = kv_pairs[i + 1];
				if (!k || !v) break;

				/* Append '&' between items */
				if (qlen > 0) {
					if (qlen + 1 < sizeof(query)) {
						query[qlen++] = '&';
						query[qlen] = '\0';
					} else {
						ret = MCPKG_NET_ERR_RANGE;
						break;
					}
				}
				/* Append k=v (URL-encoding handled by URL API when set) */
				{
					size_t need = strlen(k) + 1 + strlen(v);
					if (qlen + need >= sizeof(query)) {
						ret = MCPKG_NET_ERR_RANGE;
						break;
					}
					memcpy(query + qlen, k, strlen(k));
					qlen += strlen(k);
					query[qlen++] = '=';
					memcpy(query + qlen, v, strlen(v));
					qlen += strlen(v);
					query[qlen] = '\0';
				}
			}
			if (ret == MCPKG_NET_NO_ERROR) {
				ret = mcpkg_net_url_set_query(u, query);
				has_query = 1;
			}
		}
	}

	/* Compose final URL string */
	if (ret == MCPKG_NET_NO_ERROR) {
		char tmp[4096];
		ret = mcpkg_net_url_to_string(u, tmp, sizeof(tmp));
		if (ret == MCPKG_NET_NO_ERROR) {
			*out = dup_cstr(tmp);
			if (!*out)
				ret = MCPKG_NET_ERR_NOMEM;
		}
	}

	/* Clean up */
	if (u)
		mcpkg_net_url_free(u);
	(void)has_query; /* reserved for future logic */
	return ret;
}

static int load_offline_file(const char *root, const char *path,
                             struct McPkgNetBuf *out_body)
{
	int ret = MCPKG_NET_NO_ERROR;
	FILE *fp = NULL;
	char *full = NULL;
	long sz, rd;

	if (!root || !path || !out_body)
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR) {
		size_t need = strlen(root) + 1 + strlen(path) + 1;
		full = (char *)malloc(need);
		if (!full)
			ret = MCPKG_NET_ERR_NOMEM;
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		snprintf(full, strlen(root) + 1 + strlen(path) + 1, "%s/%s", root, path);
		fp = fopen(full, "rb");
		if (!fp)
			ret = MCPKG_NET_ERR_SYS;
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		if (fseek(fp, 0, SEEK_END) != 0)
			ret = MCPKG_NET_ERR_SYS;
	}
	if (ret == MCPKG_NET_NO_ERROR) {
		sz = ftell(fp);
		if (sz < 0)
			ret = MCPKG_NET_ERR_SYS;
	}
	if (ret == MCPKG_NET_NO_ERROR) {
		if (fseek(fp, 0, SEEK_SET) != 0)
			ret = MCPKG_NET_ERR_SYS;
	}
	if (ret == MCPKG_NET_NO_ERROR) {
		ret = mcpkg_net_buf_reserve(out_body, (size_t)sz + 1);
	}
	if (ret == MCPKG_NET_NO_ERROR) {
		rd = (long)fread(out_body->data, 1, (size_t)sz, fp);
		if (rd != sz)
			ret = MCPKG_NET_ERR_SYS;
		else {
			out_body->len = (size_t)sz;
			out_body->data[out_body->len] = '\0';
		}
	}

	if (fp) fclose(fp);
	free(full);
	return ret;
}

/* -------- public API -------- */

MCPKG_API int
mcpkg_net_global_init(void)
{
	int ret = MCPKG_NET_NO_ERROR;
	if (curl_global_init(CURL_GLOBAL_SSL) != 0)
		ret = MCPKG_NET_ERR_SYS;
	return ret;
}

MCPKG_API void
mcpkg_net_global_cleanup(void)
{
	curl_global_cleanup();
}

MCPKG_API int
mcpkg_net_client_new(McPkgNetClient **out,
                     const char *base_url,
                     const char *user_agent)
{
	int ret = MCPKG_NET_NO_ERROR;
	McPkgNetClient *c = NULL;

	if (!out || !base_url || !user_agent)
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR) {
		c = (McPkgNetClient *)calloc(1, sizeof(*c));
		if (!c)
			ret = MCPKG_NET_ERR_NOMEM;
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		c->base_url = dup_cstr(base_url);
		c->user_agent = dup_cstr(user_agent);
		if (!c->base_url || !c->user_agent)
			ret = MCPKG_NET_ERR_NOMEM;
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		c->connect_timeout_ms = 10000;
		c->total_timeout_ms = 30000;
		c->rl_limit = -1;
		c->rl_remaining = -1;
		c->rl_reset = -1;
	}

	if (ret == MCPKG_NET_NO_ERROR)
		*out = c;

	if (ret != MCPKG_NET_NO_ERROR && c) {
		free(c->base_url);
		free(c->user_agent);
		free(c);
	}
	return ret;
}

MCPKG_API void
mcpkg_net_client_free(McPkgNetClient *c)
{
	if (!c) return;
	if (c->curl) curl_easy_cleanup(c->curl);
	if (c->headers) curl_slist_free_all(c->headers);
	free(c->base_url);
	free(c->user_agent);
	free(c->offline_root);
	free(c);
}

MCPKG_API int
mcpkg_net_set_header(McPkgNetClient *c, const char *header_line)
{
	int ret = MCPKG_NET_NO_ERROR;
	struct curl_slist *h;

	if (!c || !header_line)
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR) {
		h = curl_slist_append(c->headers, header_line);
		if (!h)
			ret = MCPKG_NET_ERR_NOMEM;
		else
			c->headers = h;
	}
	if (ret == MCPKG_NET_NO_ERROR && c->curl)
		curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, c->headers);

	return ret;
}

MCPKG_API int
mcpkg_net_set_timeout(McPkgNetClient *c, long connect_ms, long total_ms)
{
	int ret = MCPKG_NET_NO_ERROR;

	if (!c || connect_ms < 0 || total_ms < 0)
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR) {
		c->connect_timeout_ms = connect_ms;
		c->total_timeout_ms = total_ms;
	}

	if (ret == MCPKG_NET_NO_ERROR && c->curl) {
		curl_easy_setopt(c->curl, CURLOPT_CONNECTTIMEOUT_MS, c->connect_timeout_ms);
		curl_easy_setopt(c->curl, CURLOPT_TIMEOUT_MS, c->total_timeout_ms);
	}
	return ret;
}

MCPKG_API int
mcpkg_net_set_offline(McPkgNetClient *c, int on_off)
{
	int ret = MCPKG_NET_NO_ERROR;
	if (!c)
		ret = MCPKG_NET_ERR_INVALID;
	if (ret == MCPKG_NET_NO_ERROR)
		c->offline = on_off ? 1 : 0;
	return ret;
}

MCPKG_API int
mcpkg_net_set_offline_root(McPkgNetClient *c, const char *dir)
{
	int ret = MCPKG_NET_NO_ERROR;
	char *dup = NULL;

	if (!c || !dir)
		ret = MCPKG_NET_ERR_INVALID;

	if (ret == MCPKG_NET_NO_ERROR) {
		dup = dup_cstr(dir);
		if (!dup)
			ret = MCPKG_NET_ERR_NOMEM;
	}
	if (ret == MCPKG_NET_NO_ERROR) {
		free(c->offline_root);
		c->offline_root = dup;
	}
	return ret;
}

MCPKG_API int
mcpkg_net_set_pool_size(McPkgNetClient *c, int n)
{
	(void)c;
	(void)n; /* future extension */
	return MCPKG_NET_NO_ERROR;
}

MCPKG_API int
mcpkg_net_request(McPkgNetClient *c,
                  MCPKG_NET_METHOD method,
                  const char *path,
                  const char *query_kv_pairs[],
                  const void *body, size_t body_len,
                  struct McPkgNetBuf *out_body,
                  long *out_http_code)
{
	int ret = MCPKG_NET_NO_ERROR;
	char *full_url = NULL;
	CURLcode cc;
	long code = 0;

	if (!c || !path || !out_body)
		ret = MCPKG_NET_ERR_INVALID;

	/* Offline mode shortcut (reads from offline_root/path) */
	if (ret == MCPKG_NET_NO_ERROR && c->offline) {
		out_body->len = 0;
		ret = load_offline_file(c->offline_root ? c->offline_root : ".", path,
		                        out_body);
		if (out_http_code) *out_http_code = 200;
		return ret;
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		ret = ensure_curl(c);
	}

	/* Build URL via URL module (handles encoding and joining) */
	if (ret == MCPKG_NET_NO_ERROR) {
		ret = build_full_url(c->base_url, path, query_kv_pairs, &full_url);
	}

	/* Perform request */
	if (ret == MCPKG_NET_NO_ERROR) {
		out_body->len = 0;
		if (c->headers)
			curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, c->headers);
		curl_easy_setopt(c->curl, CURLOPT_URL, full_url);
		curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, write_body_cb);
		curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, out_body);

		c->rl_limit = -1;
		c->rl_remaining = -1;
		c->rl_reset = -1;

		switch (method) {
			case MCPKG_NET_METHOD_GET:
				curl_easy_setopt(c->curl, CURLOPT_HTTPGET, 1L);
				curl_easy_setopt(c->curl, CURLOPT_CUSTOMREQUEST, NULL);
				curl_easy_setopt(c->curl, CURLOPT_POST, 0L);
				curl_easy_setopt(c->curl, CURLOPT_POSTFIELDS, NULL);
				curl_easy_setopt(c->curl, CURLOPT_POSTFIELDSIZE, 0L);
				break;
			case MCPKG_NET_METHOD_DELETE:
				curl_easy_setopt(c->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
				curl_easy_setopt(c->curl, CURLOPT_POST, 0L);
				curl_easy_setopt(c->curl, CURLOPT_POSTFIELDS, NULL);
				curl_easy_setopt(c->curl, CURLOPT_POSTFIELDSIZE, 0L);
				break;
			case MCPKG_NET_METHOD_POST:
				curl_easy_setopt(c->curl, CURLOPT_CUSTOMREQUEST, NULL);
				curl_easy_setopt(c->curl, CURLOPT_POST, 1L);
				curl_easy_setopt(c->curl, CURLOPT_POSTFIELDS, body ? body : "");
				curl_easy_setopt(c->curl, CURLOPT_POSTFIELDSIZE, (long)body_len);
				break;
			case MCPKG_NET_METHOD_PUT:
				curl_easy_setopt(c->curl, CURLOPT_CUSTOMREQUEST, "PUT");
				curl_easy_setopt(c->curl, CURLOPT_POST, 0L);
				curl_easy_setopt(c->curl, CURLOPT_POSTFIELDS, body ? body : "");
				curl_easy_setopt(c->curl, CURLOPT_POSTFIELDSIZE, (long)body_len);
				break;
			default:
				ret = MCPKG_NET_ERR_INVALID;
				break;
		}
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		cc = curl_easy_perform(c->curl);
		if (cc != CURLE_OK) {
			if (cc == CURLE_OPERATION_TIMEDOUT)
				ret = MCPKG_NET_ERR_TIMEOUT;
			else
				ret = MCPKG_NET_ERR_SYS;
		}
	}

	if (ret == MCPKG_NET_NO_ERROR) {
		curl_easy_getinfo(c->curl, CURLINFO_RESPONSE_CODE, &code);
		c->last_http_code = code;
		if (out_http_code) *out_http_code = code;
		if (code < 200 || code >= 300)
			ret = MCPKG_NET_ERR_PROTO; /* map non-2xx to protocol error */
	}

	free(full_url);
	return ret;
}

MCPKG_API int
mcpkg_net_get_ratelimit(McPkgNetClient *c, McPkgNetRateLimit *out)
{
	int ret = MCPKG_NET_NO_ERROR;
	if (!c || !out)
		ret = MCPKG_NET_ERR_INVALID;
	if (ret == MCPKG_NET_NO_ERROR) {
		out->limit = c->rl_limit;
		out->remaining = c->rl_remaining;
		out->reset = c->rl_reset;
	}
	return ret;
}
