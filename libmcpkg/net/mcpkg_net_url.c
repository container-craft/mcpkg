/* SPDX-License-Identifier: MIT */
#include "mcpkg_net_url.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <curl/curl.h>
#include <curl/urlapi.h>

struct McPkgNetUrl {
    CURLU *h;
};

static int curlu2neterr(CURLUcode cu)
{
    switch (cu) {
    case CURLUE_OK: return MCPKG_NET_NO_ERROR;
    case CURLUE_OUT_OF_MEMORY: return MCPKG_NET_ERR_NOMEM;
    default: return MCPKG_NET_ERR_PROTO;
    }
}

MCPKG_API McPkgNetUrl *mcpkg_net_url_new(void)
{
    McPkgNetUrl *u = (McPkgNetUrl *)calloc(1, sizeof(*u));
    if (!u)
        return NULL;
    u->h = curl_url();
    if (!u->h) {
        free(u);
        return NULL;
    }
    return u;
}

MCPKG_API void mcpkg_net_url_free(McPkgNetUrl *u)
{
    if (!u) return;
    if (u->h) curl_url_cleanup(u->h);
    free(u);
}

MCPKG_API McPkgNetUrl *mcpkg_net_url_clone(McPkgNetUrl *u)
{
    if (!u || !u->h)
        return NULL;

    McPkgNetUrl *v = (McPkgNetUrl *)calloc(1, sizeof(*v));
    if (!v)
        return NULL;

    v->h = curl_url_dup(u->h);   /* ← correct: returns CURLU* */
    if (!v->h) {
        free(v);
        return NULL;            /* OOM or invalid input */
    }
    return v;
}

MCPKG_API int mcpkg_net_url_clear(McPkgNetUrl *u)
{
    if (!u || !u->h) return MCPKG_NET_ERR_INVALID;
    (void)curl_url_set(u->h, CURLUPART_SCHEME,   NULL, 0);
    (void)curl_url_set(u->h, CURLUPART_USER,     NULL, 0);
    (void)curl_url_set(u->h, CURLUPART_PASSWORD, NULL, 0);
    (void)curl_url_set(u->h, CURLUPART_HOST,     NULL, 0);
    (void)curl_url_set(u->h, CURLUPART_PORT,     NULL, 0);
    (void)curl_url_set(u->h, CURLUPART_PATH,     NULL, 0);
    (void)curl_url_set(u->h, CURLUPART_QUERY,    NULL, 0);
    (void)curl_url_set(u->h, CURLUPART_FRAGMENT, NULL, 0);
    return MCPKG_NET_NO_ERROR;
}

MCPKG_API int mcpkg_net_url_parse(McPkgNetUrl *u, const char *url_utf8)
{
    if (!u || !u->h || !url_utf8) return MCPKG_NET_ERR_INVALID;
    return curlu2neterr(curl_url_set(u->h, CURLUPART_URL, url_utf8, CURLU_GUESS_SCHEME));
}

/* ---------- helpers ---------- */

static int part_to_buf(CURLU *h, CURLUPart part, unsigned flags, char *buf, size_t buf_sz)
{
    char *tmp = NULL;
    CURLUcode cu;

    if (!h || !buf || buf_sz == 0)
        return MCPKG_NET_ERR_INVALID;

    cu = curl_url_get(h, part, &tmp, flags);
    if (cu == CURLUE_OK) {
        size_t n = strlen(tmp);
        if (n + 1 > buf_sz) {
            curl_free(tmp);
            return MCPKG_NET_ERR_RANGE;
        }
        memcpy(buf, tmp, n + 1);
        curl_free(tmp);
        return MCPKG_NET_NO_ERROR;
    }

    /* treat these as empty (not present / lacks IDN) */
    if (cu == CURLUE_NO_SCHEME || cu == CURLUE_NO_USER || cu == CURLUE_NO_PASSWORD ||
        cu == CURLUE_NO_OPTIONS || cu == CURLUE_NO_HOST || cu == CURLUE_NO_PORT ||
        cu == CURLUE_NO_QUERY  || cu == CURLUE_NO_FRAGMENT || cu == CURLUE_LACKS_IDN) {
        buf[0] = '\0';
        return MCPKG_NET_NO_ERROR;
    }

    return curlu2neterr(cu);
}

static int has_part(CURLU *h, CURLUPart part, unsigned flags, int *out_yes)
{
    char *tmp = NULL;
    CURLUcode cu;

    if (!h || !out_yes)
        return MCPKG_NET_ERR_INVALID;

    cu = curl_url_get(h, part, &tmp, flags);
    if (cu == CURLUE_OK) {
        *out_yes = (tmp && tmp[0]) ? 1 : 0;
        curl_free(tmp);
        return MCPKG_NET_NO_ERROR;
    }
    if (cu == CURLUE_NO_SCHEME || cu == CURLUE_NO_USER || cu == CURLUE_NO_PASSWORD ||
        cu == CURLUE_NO_OPTIONS || cu == CURLUE_NO_HOST || cu == CURLUE_NO_PORT ||
        cu == CURLUE_NO_QUERY  || cu == CURLUE_NO_FRAGMENT || cu == CURLUE_LACKS_IDN) {
        *out_yes = 0;
        return MCPKG_NET_NO_ERROR;
    }
    return curlu2neterr(cu);
}

/* ---------- predicates / getters ---------- */

MCPKG_API int mcpkg_net_url_is_empty(McPkgNetUrl *u, int *out_is_empty)
{
    int yes = 0;
    char path[8];

    if (!u || !out_is_empty) return MCPKG_NET_ERR_INVALID;

    if (has_part(u->h, CURLUPART_SCHEME, 0, &yes) != MCPKG_NET_NO_ERROR) return MCPKG_NET_ERR_PROTO;
    if (yes) { *out_is_empty = 0; return MCPKG_NET_NO_ERROR; }

    if (has_part(u->h, CURLUPART_HOST, 0, &yes) != MCPKG_NET_NO_ERROR) return MCPKG_NET_ERR_PROTO;
    if (yes) { *out_is_empty = 0; return MCPKG_NET_NO_ERROR; }

    if (part_to_buf(u->h, CURLUPART_PATH, 0, path, sizeof(path)) != MCPKG_NET_NO_ERROR) return MCPKG_NET_ERR_PROTO;
    if (path[0] && !(path[0] == '/' && path[1] == '\0')) {
        *out_is_empty = 0; return MCPKG_NET_NO_ERROR;
    }

    if (has_part(u->h, CURLUPART_QUERY, 0, &yes) != MCPKG_NET_NO_ERROR) return MCPKG_NET_ERR_PROTO;
    if (yes) { *out_is_empty = 0; return MCPKG_NET_NO_ERROR; }

    if (has_part(u->h, CURLUPART_FRAGMENT, 0, &yes) != MCPKG_NET_NO_ERROR) return MCPKG_NET_ERR_PROTO;
    *out_is_empty = yes ? 0 : 1;
    return MCPKG_NET_NO_ERROR;
}

MCPKG_API int mcpkg_net_url_has_query(McPkgNetUrl *u, int *out_has_query)
{
    if (!u || !out_has_query) return MCPKG_NET_ERR_INVALID;
    return has_part(u->h, CURLUPART_QUERY, 0, out_has_query);
}

MCPKG_API int mcpkg_net_url_has_fragment(McPkgNetUrl *u, int *out_has_fragment)
{
    if (!u || !out_has_fragment) return MCPKG_NET_ERR_INVALID;
    return has_part(u->h, CURLUPART_FRAGMENT, 0, out_has_fragment);
}

MCPKG_API int mcpkg_net_url_scheme(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_SCHEME, 0, buf, buf_sz);
}
MCPKG_API int mcpkg_net_url_host(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_HOST, 0, buf, buf_sz);
}
MCPKG_API int mcpkg_net_url_host_ascii(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    if (!u || !buf || buf_sz == 0) return MCPKG_NET_ERR_INVALID;
    char *tmp = NULL;
    CURLUcode cu = curl_url_get(u->h, CURLUPART_HOST, &tmp, CURLU_PUNYCODE);
    if (cu == CURLUE_OK) {
        size_t n = strlen(tmp);
        if (n + 1 > buf_sz) { curl_free(tmp); return MCPKG_NET_ERR_RANGE; }
        memcpy(buf, tmp, n + 1);
        curl_free(tmp);
        return MCPKG_NET_NO_ERROR;
    }
    if (cu == CURLUE_OUT_OF_MEMORY) return MCPKG_NET_ERR_NOMEM;
    return mcpkg_net_url_host(u, buf, buf_sz);
}
MCPKG_API int mcpkg_net_url_port(McPkgNetUrl *u, int *out_port)
{
    char tmp[16];
    long p;
    if (!u || !out_port) return MCPKG_NET_ERR_INVALID;
    if (part_to_buf(u->h, CURLUPART_PORT, 0, tmp, sizeof(tmp)) != MCPKG_NET_NO_ERROR) return MCPKG_NET_ERR_PROTO;
    if (!tmp[0]) { *out_port = 0; return MCPKG_NET_NO_ERROR; }
    p = strtol(tmp, NULL, 10);
    if (p < 0 || p > 65535) return MCPKG_NET_ERR_RANGE;
    *out_port = (int)p;
    return MCPKG_NET_NO_ERROR;
}
MCPKG_API int mcpkg_net_url_path(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_PATH, 0, buf, buf_sz);
}
MCPKG_API int mcpkg_net_url_query(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_QUERY, 0, buf, buf_sz);
}
MCPKG_API int mcpkg_net_url_fragment(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_FRAGMENT, 0, buf, buf_sz);
}

/* ---------- setters ---------- */

static int set_part(McPkgNetUrl *u, CURLUPart part, const char *val, unsigned flags)
{
    if (!u || !u->h) return MCPKG_NET_ERR_INVALID;
    return curlu2neterr(curl_url_set(u->h, part, val, flags));
}

MCPKG_API int mcpkg_net_url_set_scheme(McPkgNetUrl *u, const char *scheme_ascii)
{
    return set_part(u, CURLUPART_SCHEME, scheme_ascii, 0);
}
MCPKG_API int mcpkg_net_url_set_host(McPkgNetUrl *u, const char *host_utf8)
{
    return set_part(u, CURLUPART_HOST, host_utf8, 0);
}
MCPKG_API int mcpkg_net_url_set_port(McPkgNetUrl *u, int port)
{
    char tmp[16];
    if (!u || !u->h || port < 0 || port > 65535) return MCPKG_NET_ERR_INVALID;
    if (port == 0) { (void)curl_url_set(u->h, CURLUPART_PORT, NULL, 0); return MCPKG_NET_NO_ERROR; }
    snprintf(tmp, sizeof(tmp), "%d", port);
    return set_part(u, CURLUPART_PORT, tmp, 0);
}
MCPKG_API int mcpkg_net_url_set_path(McPkgNetUrl *u, const char *path_utf8)
{
    /* NOTE: We rely on CURLU_URLENCODE to escape path segments. */
    return set_part(u, CURLUPART_PATH, path_utf8, CURLU_URLENCODE);
}
MCPKG_API int mcpkg_net_url_set_password(McPkgNetUrl *u, const char *password_utf8)
{
    return set_part(u, CURLUPART_PASSWORD, password_utf8, CURLU_URLENCODE);
}
MCPKG_API int mcpkg_net_url_set_query(McPkgNetUrl *u, const char *query_no_qmark)
{
    /* Caller passes raw 'a=b&c=d' (no '?'). */
    return set_part(u, CURLUPART_QUERY, query_no_qmark, 0);
}
MCPKG_API int mcpkg_net_url_add_query(McPkgNetUrl *u, const char *key_utf8, const char *val_utf8)
{
    /* libcurl expects "k=v" with CURLU_APPENDQUERY; URLENCODE ensures proper escaping */
    char *kv;
    size_t klen = key_utf8 ? strlen(key_utf8) : 0;
    size_t vlen = val_utf8 ? strlen(val_utf8) : 0;

    if (!u || !u->h || !key_utf8 || !val_utf8)
        return MCPKG_NET_ERR_INVALID;

    kv = (char *)malloc(klen + 1 + vlen + 1);
    if (!kv) return MCPKG_NET_ERR_NOMEM;

    memcpy(kv, key_utf8, klen);
    kv[klen] = '=';
    memcpy(kv + klen + 1, val_utf8, vlen);
    kv[klen + 1 + vlen] = '\0';

    {
        CURLUcode cu = curl_url_set(u->h, CURLUPART_QUERY, kv, CURLU_APPENDQUERY | CURLU_URLENCODE);
        free(kv);
        return curlu2neterr(cu);
    }
}
MCPKG_API int mcpkg_net_url_set_fragment(McPkgNetUrl *u, const char *fragment_no_hash)
{
    return set_part(u, CURLUPART_FRAGMENT, fragment_no_hash, CURLU_URLENCODE);
}

/* ---------- to-string ---------- */

MCPKG_API int mcpkg_net_url_to_string_buf(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    if (!u || !u->h || !buf || buf_sz == 0) return MCPKG_NET_ERR_INVALID;
    return part_to_buf(u->h, CURLUPART_URL, 0, buf, buf_sz);
}

MCPKG_API int mcpkg_net_url_to_string(McPkgNetUrl *u, char **out)
{
    char *tmp = NULL;
    size_t n;
    char *dst;

    if (!u || !u->h || !out) return MCPKG_NET_ERR_INVALID;

    {
        CURLUcode cu = curl_url_get(u->h, CURLUPART_URL, &tmp, 0);
        if (cu != CURLUE_OK)
            return curlu2neterr(cu);
    }

    n = strlen(tmp);
    dst = (char *)malloc(n + 1);
    if (!dst) { curl_free(tmp); return MCPKG_NET_ERR_NOMEM; }

    memcpy(dst, tmp, n + 1);
    curl_free(tmp);

    *out = dst;
    return MCPKG_NET_NO_ERROR;
}
