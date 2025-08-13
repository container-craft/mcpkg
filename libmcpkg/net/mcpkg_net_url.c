#include "mcpkg_net_url.h"

#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
// net/mcpkg_net_url.c imply -> libcurl -> curl/urlapi.h
#include <curl/urlapi.h>
#include "mcpkg_net_util.h"

struct McPkgNetUrl {
    CURLU *h;
};

static int curlu2neterr(CURLUcode cu)
{
    int ret = MCPKG_NET_NO_ERROR;

    if (cu == CURLUE_OK)
        ret = MCPKG_NET_NO_ERROR;
    else if (cu == CURLUE_OUT_OF_MEMORY)
        ret = MCPKG_NET_ERR_NOMEM;
    else if (cu == CURLUE_NO_SCHEME || cu == CURLUE_NO_USER || cu == CURLUE_NO_PASSWORD ||
             cu == CURLUE_NO_OPTIONS || cu == CURLUE_NO_HOST || cu == CURLUE_NO_PORT ||
             cu == CURLUE_NO_QUERY  || cu == CURLUE_NO_FRAGMENT ||
             cu == CURLUE_LACKS_IDN) {
        /* “not present” parts and lack of IDN support are non-fatal in getters/preds */
        ret = MCPKG_NET_NO_ERROR;
    } else {
        ret = MCPKG_NET_ERR_PROTO;
    }
    return ret;
}

MCPKG_API int
mcpkg_net_url_new(McPkgNetUrl **out)
{
    int ret = MCPKG_NET_NO_ERROR;
    McPkgNetUrl *u = NULL;

    if (!out)
        ret = MCPKG_NET_ERR_INVALID;

    if (ret == MCPKG_NET_NO_ERROR) {
        u = (McPkgNetUrl *)calloc(1, sizeof(*u));
        if (!u)
            ret = MCPKG_NET_ERR_NOMEM;
    }

    if (ret == MCPKG_NET_NO_ERROR) {
        u->h = curl_url();
        if (!u->h) {
            free(u);
            ret = MCPKG_NET_ERR_SYS;
        }
    }

    if (ret == MCPKG_NET_NO_ERROR)
        *out = u;
    return ret;
}

MCPKG_API void
mcpkg_net_url_free(McPkgNetUrl *u)
{
    if (!u)
        return;
    if (u->h)
        curl_url_cleanup(u->h);

    free(u);
}

MCPKG_API int
mcpkg_net_url_clear(McPkgNetUrl *u)
{
    int ret = MCPKG_NET_NO_ERROR;
    if (!u || !u->h)
        ret = MCPKG_NET_ERR_INVALID;

    if (ret == MCPKG_NET_NO_ERROR) {
        (void)curl_url_set(u->h, CURLUPART_SCHEME,   NULL, 0);
        (void)curl_url_set(u->h, CURLUPART_USER,     NULL, 0);
        (void)curl_url_set(u->h, CURLUPART_PASSWORD, NULL, 0);
        (void)curl_url_set(u->h, CURLUPART_HOST,     NULL, 0);
        (void)curl_url_set(u->h, CURLUPART_PORT,     NULL, 0);
        (void)curl_url_set(u->h, CURLUPART_PATH,     NULL, 0);
        (void)curl_url_set(u->h, CURLUPART_QUERY,    NULL, 0);
        (void)curl_url_set(u->h, CURLUPART_FRAGMENT, NULL, 0);
    }
    return ret;
}

MCPKG_API int
mcpkg_net_url_set_url(McPkgNetUrl *u, const char *url)
{
    int ret = MCPKG_NET_NO_ERROR;
    if (!u || !u->h || !url)
        ret = MCPKG_NET_ERR_INVALID;
    if (ret == MCPKG_NET_NO_ERROR)
        ret = curlu2neterr(curl_url_set(u->h, CURLUPART_URL, url, CURLU_GUESS_SCHEME));
    return ret;
}

static int
part_to_buf(CURLU *h, CURLUPart part, unsigned flags, char *buf, size_t buf_sz)
{
    int ret = MCPKG_NET_NO_ERROR;
    char *tmp = NULL;
    CURLUcode cu;

    if (!h || !buf || buf_sz == 0)
        return MCPKG_NET_ERR_INVALID;

    cu = curl_url_get(h, part, &tmp, flags);
    if (cu == CURLUE_OK) {
        size_t n = strlen(tmp);
        if (n + 1 > buf_sz) {
            ret = MCPKG_NET_ERR_RANGE;
        } else {
            memcpy(buf, tmp, n + 1);
        }
        curl_free(tmp);
        return ret;
    }

    /* treat “not set” and lacks-IDN as empty string */
    if (cu == CURLUE_NO_SCHEME || cu == CURLUE_NO_USER || cu == CURLUE_NO_PASSWORD ||
        cu == CURLUE_NO_OPTIONS || cu == CURLUE_NO_HOST || cu == CURLUE_NO_PORT ||
        cu == CURLUE_NO_QUERY  || cu == CURLUE_NO_FRAGMENT || cu == CURLUE_LACKS_IDN) {
        buf[0] = '\0';
        return MCPKG_NET_NO_ERROR;
    }

    return curlu2neterr(cu);
}
/* Predicates should NOT depend on tiny buffers; check presence by code. */
static int has_part(CURLU *h, CURLUPart part, unsigned flags, int *out_yes)
{
    int ret = MCPKG_NET_NO_ERROR;
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

MCPKG_API int
mcpkg_net_url_is_empty(McPkgNetUrl *u, int *out_is_empty)
{
    int ret = MCPKG_NET_NO_ERROR;
    int yes = 0;
    char path[8];

    if (!u || !out_is_empty)
        return MCPKG_NET_ERR_INVALID;

    /* scheme present? */
    ret = has_part(u->h, CURLUPART_SCHEME, 0, &yes);
    if (ret != MCPKG_NET_NO_ERROR) return ret;
    if (yes) { *out_is_empty = 0; return MCPKG_NET_NO_ERROR; }

    /* host present? */
    ret = has_part(u->h, CURLUPART_HOST, 0, &yes);
    if (ret != MCPKG_NET_NO_ERROR) return ret;
    if (yes) { *out_is_empty = 0; return MCPKG_NET_NO_ERROR; }

    /* path present? treat "" and "/" as empty */
    ret = part_to_buf(u->h, CURLUPART_PATH, 0, path, sizeof(path));
    if (ret != MCPKG_NET_NO_ERROR) return ret;
    if (path[0] && !(path[0] == '/' && path[1] == '\0')) {
        *out_is_empty = 0;
        return MCPKG_NET_NO_ERROR;
    }

    /* query present? */
    ret = has_part(u->h, CURLUPART_QUERY, 0, &yes);
    if (ret != MCPKG_NET_NO_ERROR) return ret;
    if (yes) { *out_is_empty = 0; return MCPKG_NET_NO_ERROR; }

    /* fragment present? */
    ret = has_part(u->h, CURLUPART_FRAGMENT, 0, &yes);
    if (ret != MCPKG_NET_NO_ERROR) return ret;
    if (yes) { *out_is_empty = 0; return MCPKG_NET_NO_ERROR; }

    *out_is_empty = 1;
    return MCPKG_NET_NO_ERROR;
}



MCPKG_API int
mcpkg_net_url_has_query(McPkgNetUrl *u, int *out_has_query)
{
    if (!u || !out_has_query)
        return MCPKG_NET_ERR_INVALID;

    return has_part(u->h, CURLUPART_QUERY, 0, out_has_query);
}

MCPKG_API int
mcpkg_net_url_has_fragment(McPkgNetUrl *u, int *out_has_fragment)
{
    if (!u || !out_has_fragment)
        return MCPKG_NET_ERR_INVALID;
    return has_part(u->h, CURLUPART_FRAGMENT, 0, out_has_fragment);
}

MCPKG_API int
mcpkg_net_url_scheme(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_SCHEME, 0, buf, buf_sz);
}

MCPKG_API int
mcpkg_net_url_path(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_PATH, 0, buf, buf_sz);
}

MCPKG_API int
mcpkg_net_url_password(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_PASSWORD, 0, buf, buf_sz);
}

MCPKG_API int
mcpkg_net_url_query(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_QUERY, 0, buf, buf_sz);
}

MCPKG_API int
mcpkg_net_url_fragment(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_FRAGMENT, 0, buf, buf_sz);
}

MCPKG_API int
mcpkg_net_url_host(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    return part_to_buf(u ? u->h : NULL, CURLUPART_HOST, 0, buf, buf_sz);
}

MCPKG_API int
mcpkg_net_url_host_ascii(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    if (!u || !buf || buf_sz == 0)
        return MCPKG_NET_ERR_INVALID;

    char *tmp = NULL;
    CURLUcode cu = curl_url_get(u->h, CURLUPART_HOST, &tmp, CURLU_PUNYCODE);

    if (cu == CURLUE_OK) {
        size_t n = strlen(tmp);
        if (n + 1 > buf_sz) { curl_free(tmp); return MCPKG_NET_ERR_RANGE; }
        memcpy(buf, tmp, n + 1);
        curl_free(tmp);
        return MCPKG_NET_NO_ERROR;
    }

    /* Hard error only if truly OOM; otherwise treat as "punycode unsupported" and fallback */
    if (cu == CURLUE_OUT_OF_MEMORY)
        return MCPKG_NET_ERR_NOMEM;

    /* Fallback to UTF-8 host; if host is unset this returns success with "" */
    return mcpkg_net_url_host(u, buf, buf_sz);
}


MCPKG_API int
mcpkg_net_url_port(McPkgNetUrl *u, int *out_port)
{
    int ret = MCPKG_NET_NO_ERROR;
    char tmp[16];

    if (!u || !out_port)
        return MCPKG_NET_ERR_INVALID;

    ret = part_to_buf(u->h, CURLUPART_PORT, 0, tmp, sizeof(tmp));
    if (ret != MCPKG_NET_NO_ERROR)
        return ret;

    if (!tmp[0]) {
        *out_port = 0;
        return MCPKG_NET_NO_ERROR;
    }
    {
        long p = strtol(tmp, NULL, 10);
        if (p < 0 || p > 65535)
            return MCPKG_NET_ERR_RANGE;
        *out_port = (int)p;
    }
    return MCPKG_NET_NO_ERROR;
}


static int set_part(McPkgNetUrl *u, CURLUPart part, const char *val, unsigned flags)
{
    if (!u || !u->h)
        return MCPKG_NET_ERR_INVALID;

    return curlu2neterr(curl_url_set(u->h, part, val, flags));
}

MCPKG_API int
mcpkg_net_url_set_scheme(McPkgNetUrl *u, const char *scheme)
{
    return set_part(u, CURLUPART_SCHEME, scheme, 0);
}

MCPKG_API int
mcpkg_net_url_set_path(McPkgNetUrl *u, const char *path_utf8)
{
    /// WARNING URL-encode path segments are needed
    return set_part(u, CURLUPART_PATH, path_utf8, CURLU_URLENCODE);
}

MCPKG_API int
mcpkg_net_url_set_password(McPkgNetUrl *u, const char *password_utf8)
{
    return set_part(u, CURLUPART_PASSWORD, password_utf8, CURLU_URLENCODE);
}

MCPKG_API int
mcpkg_net_url_set_query(McPkgNetUrl *u, const char *query_utf8)
{
    // query no '?'
    return set_part(u, CURLUPART_QUERY, query_utf8, 0);
    }

MCPKG_API int
mcpkg_net_url_set_fragment(McPkgNetUrl *u, const char *fragment_utf8)
{
    // fragment no #
    return set_part(u, CURLUPART_FRAGMENT, fragment_utf8, CURLU_URLENCODE);
}

MCPKG_API int
mcpkg_net_url_set_host(McPkgNetUrl *u, const char *host_utf8)
{
    return set_part(u, CURLUPART_HOST, host_utf8, 0);
}

MCPKG_API int
mcpkg_net_url_set_port(McPkgNetUrl *u, int port)
{
    if (!u || !u->h || port < 0 || port > 65535)
        return MCPKG_NET_ERR_INVALID;

    if (port == 0) {
        (void)curl_url_set(u->h, CURLUPART_PORT, NULL, 0);
        return MCPKG_NET_NO_ERROR;
    }
    {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%d", port);
        return set_part(u, CURLUPART_PORT, tmp, 0);
    }
}

MCPKG_API int
mcpkg_net_url_to_string(McPkgNetUrl *u, char *buf, size_t buf_sz)
{
    if (!u || !u->h || !buf || buf_sz == 0)
        return MCPKG_NET_ERR_INVALID;

    char *tmp = NULL;
    CURLUcode cu = curl_url_get(u->h, CURLUPART_URL, &tmp, 0);
    int ret = curlu2neterr(cu);
    if (ret != MCPKG_NET_NO_ERROR)
        return ret;

    size_t n = strlen(tmp);
    if (n + 1 > buf_sz) {
        curl_free(tmp);
        return MCPKG_NET_ERR_RANGE;
    }
    memcpy(buf, tmp, n + 1);
    curl_free(tmp);
    return MCPKG_NET_NO_ERROR;
}
