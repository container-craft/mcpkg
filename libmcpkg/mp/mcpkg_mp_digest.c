/* SPDX-License-Identifier: MIT */
#include "mcpkg_mp_digest.h"

#include <stdlib.h>
#include <string.h>

#include <msgpack.h>

/* Tag/version and int keys */
#define DIGEST_TAG "digest"
#define DIGEST_VER 1

#define K_ALGO 2
#define K_HEX  3

/* ---- small helpers ---- */
static size_t expected_hex_len(MCPKG_DIGEST_ALGO a)
{
    switch (a) {
    case MCPKG_DIGEST_SHA1:   return 40;
    case MCPKG_DIGEST_SHA256: return 64;
    case MCPKG_DIGEST_SHA512: return 128;
    default: return 0;
    }
}

static int is_hexstr(const char *s, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F')))
            return 0;
    }
    return 1;
}

/* Borrow the root msgpack_object safely */
static const msgpack_object *root_obj(const struct McPkgMpReader *r)
{
    return (const msgpack_object *)r->root;
}

/* Find int-key in a map object (borrowed) */
static int find_map_key(const msgpack_object *map, int key,
                        const msgpack_object **out, int *found)
{
    size_t i;
    if (!map || !out || !found) return MCPKG_MP_ERR_INVALID_ARG;
    *found = 0;
    if (map->type != MSGPACK_OBJECT_MAP) return MCPKG_MP_ERR_PARSE;

    for (i = 0; i < map->via.map.size; i++) {
        const msgpack_object *k = &map->via.map.ptr[i].key;
        if (k->type == MSGPACK_OBJECT_POSITIVE_INTEGER ||
            k->type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
            if ((int)k->via.i64 == key) {
                *out = &map->via.map.ptr[i].val;
                *found = 1;
                return MCPKG_MP_NO_ERROR;
            }
        }
    }
    return MCPKG_MP_NO_ERROR;
}

/* Parse a digest from a map object (borrowed) */
static int parse_digest_map(const msgpack_object *map, McPkgDigest *out)
{
    int ret = MCPKG_MP_NO_ERROR;
    const msgpack_object *v;
    int found;

    if (!map || !out) return MCPKG_MP_ERR_INVALID_ARG;
    if (map->type != MSGPACK_OBJECT_MAP) return MCPKG_MP_ERR_PARSE;

    /* tag */
    ret = mcpkg_mp_expect_tag((const struct McPkgMpReader *)NULL, NULL, NULL);
    /* We can’t use expect_tag directly on submaps; re-check manually */
    /* Check key 0 -> "digest" */
    found = 0; v = NULL;
    ret = find_map_key(map, MCPKG_MP_K_TAG, &v, &found);
    if (ret != MCPKG_MP_NO_ERROR) return ret;
    if (!found || v->type != MSGPACK_OBJECT_STR ||
        v->via.str.size != (uint32_t)sizeof(DIGEST_TAG)-1 ||
        memcmp(v->via.str.ptr, DIGEST_TAG, sizeof(DIGEST_TAG)-1) != 0)
        return MCPKG_MP_ERR_PARSE;

    /* version (optional) */
    found = 0; v = NULL;
    ret = find_map_key(map, MCPKG_MP_K_VER, &v, &found);
    if (ret != MCPKG_MP_NO_ERROR) return ret;
    if (found) {
        if (!(v->type == MSGPACK_OBJECT_POSITIVE_INTEGER || v->type == MSGPACK_OBJECT_NEGATIVE_INTEGER))
            return MCPKG_MP_ERR_PARSE;
        if ((int)v->via.i64 < 1) return MCPKG_MP_ERR_PARSE;
    }

    /* algo */
    found = 0; v = NULL;
    ret = find_map_key(map, K_ALGO, &v, &found);
    if (ret != MCPKG_MP_NO_ERROR) return ret;
    if (!found || !(v->type == MSGPACK_OBJECT_POSITIVE_INTEGER || v->type == MSGPACK_OBJECT_NEGATIVE_INTEGER))
        return MCPKG_MP_ERR_PARSE;

    out->algo = (MCPKG_DIGEST_ALGO)(int)v->via.i64;

    /* hex */
    found = 0; v = NULL;
    ret = find_map_key(map, K_HEX, &v, &found);
    if (ret != MCPKG_MP_NO_ERROR) return ret;
    if (!found || v->type != MSGPACK_OBJECT_STR)
        return MCPKG_MP_ERR_PARSE;

    out->hex = (char *)malloc(v->via.str.size + 1);
    if (!out->hex) return MCPKG_MP_ERR_NO_MEMORY;
    memcpy(out->hex, v->via.str.ptr, v->via.str.size);
    out->hex[v->via.str.size] = '\0';

    /* validate */
    ret = mcpkg_digest_validate(out);
    if (ret != MCPKG_MP_NO_ERROR) {
        mcpkg_digest_free(out);
        return ret;
    }
    return MCPKG_MP_NO_ERROR;
}

/* ---- public: lifecycle ---- */

int mcpkg_digest_init(McPkgDigest *d)
{
    if (!d) return MCPKG_MP_ERR_INVALID_ARG;
    d->algo = (MCPKG_DIGEST_ALGO)0;
    d->hex = NULL;
    return MCPKG_MP_NO_ERROR;
}

void mcpkg_digest_free(McPkgDigest *d)
{
    if (!d) return;
    free(d->hex);
    d->hex = NULL;
    d->algo = (MCPKG_DIGEST_ALGO)0;
}

int mcpkg_digest_copy(McPkgDigest *dst, const McPkgDigest *src)
{
    if (!dst || !src) return MCPKG_MP_ERR_INVALID_ARG;
    dst->algo = src->algo;
    dst->hex = NULL;
    if (src->hex) {
        dst->hex = strdup(src->hex);
        if (!dst->hex) return MCPKG_MP_ERR_NO_MEMORY;
    }
    return MCPKG_MP_NO_ERROR;
}

int mcpkg_digest_validate(const McPkgDigest *d)
{
    size_t want;
    size_t n;
    if (!d || !d->hex) return MCPKG_MP_ERR_INVALID_ARG;

    want = expected_hex_len(d->algo);
    if (!want) return MCPKG_MP_ERR_PARSE;

    n = strlen(d->hex);
    if (n != want) return MCPKG_MP_ERR_PARSE;
    if (!is_hexstr(d->hex, n)) return MCPKG_MP_ERR_PARSE;

    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_write_digest(struct McPkgMpWriter *w, const McPkgDigest *d)
{
    int ret = MCPKG_MP_NO_ERROR;
    if (!w || !d)
        return MCPKG_MP_ERR_INVALID_ARG;

    ret = mcpkg_digest_validate(d);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    /* tag+ver+algo+hex = 4 entries */
    ret = mcpkg_mp_map_begin(w, 4);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    ret = mcpkg_mp_write_header(w, DIGEST_TAG, DIGEST_VER);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    ret = mcpkg_mp_kv_u32(w, K_ALGO, (uint32_t)d->algo);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    ret = mcpkg_mp_kv_str(w, K_HEX, d->hex);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_kv_digest(struct McPkgMpWriter *w, int key, const McPkgDigest *d)
{
    int ret = MCPKG_MP_NO_ERROR;
    if (!w || !d) return MCPKG_MP_ERR_INVALID_ARG;

    ret = mcpkg_digest_validate(d);
    if (ret != MCPKG_MP_NO_ERROR) return ret;

    /* Begin the value map under 'key' */
    ret = mcpkg_mp_kv_map_begin(w, key, 4);
    if (ret != MCPKG_MP_NO_ERROR) return ret;

    ret = mcpkg_mp_write_header(w, DIGEST_TAG, DIGEST_VER);
    if (ret != MCPKG_MP_NO_ERROR) return ret;

    ret = mcpkg_mp_kv_u32(w, K_ALGO, (uint32_t)d->algo);
    if (ret != MCPKG_MP_NO_ERROR) return ret;

    ret = mcpkg_mp_kv_str(w, K_HEX, d->hex);
    if (ret != MCPKG_MP_NO_ERROR) return ret;

    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_read_digest(const struct McPkgMpReader *r, McPkgDigest *out)
{
    int ret = MCPKG_MP_NO_ERROR;
    int version = 0;
    const msgpack_object *root;

    if (!r || !r->impl || !out) return MCPKG_MP_ERR_INVALID_ARG;

    /* Validate the tag on top-level root */
    ret = mcpkg_mp_expect_tag(r, DIGEST_TAG, &version);
    if (ret != MCPKG_MP_NO_ERROR) return ret;
    if (version < 1) return MCPKG_MP_ERR_PARSE;

    /* Parse from the root map */
    root = root_obj(r);
    return parse_digest_map(root, out);
}

int mcpkg_mp_get_digest_dup(const struct McPkgMpReader *r, int key, McPkgDigest *out, int *found)
{
    const msgpack_object *root;
    const msgpack_object *val = NULL;
    int ret = MCPKG_MP_NO_ERROR;
    int f = 0;

    if (found) *found = 0;
    if (!r || !r->impl || !out) return MCPKG_MP_ERR_INVALID_ARG;

    root = root_obj(r);
    ret = find_map_key(root, key, &val, &f);
    if (ret != MCPKG_MP_NO_ERROR) return ret;
    if (!f) return MCPKG_MP_NO_ERROR;

    if (!val || val->type != MSGPACK_OBJECT_MAP)
        return MCPKG_MP_ERR_PARSE;

    ret = parse_digest_map(val, out);
    if (ret == MCPKG_MP_NO_ERROR && found) *found = 1;
    return ret;
}

/* ---- arrays of digests under a key ---- */

int mcpkg_mp_kv_digest_list(struct McPkgMpWriter *w, int key,
                            const McPkgDigest *arr, size_t n)
{
    int ret = MCPKG_MP_NO_ERROR;
    size_t i;

    if (!w) return MCPKG_MP_ERR_INVALID_ARG;

    ret = mcpkg_mp_kv_array_begin(w, key, (uint32_t)n);
    if (ret != MCPKG_MP_NO_ERROR) return ret;

    for (i = 0; i < n; i++) {
        ret = mcpkg_mp_write_digest(w, &arr[i]);
        if (ret != MCPKG_MP_NO_ERROR) return ret;
    }
    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_get_digest_list_dup(const struct McPkgMpReader *r, int key,
                                 McPkgDigest **out_arr, size_t *out_n)
{
    const msgpack_object *root;
    const msgpack_object *val = NULL;
    int ret = MCPKG_MP_NO_ERROR, f = 0;
    McPkgDigest *vec = NULL;
    size_t i, n;

    if (!r || !r->impl || !out_arr || !out_n)
        return MCPKG_MP_ERR_INVALID_ARG;

    *out_arr = NULL;
    *out_n = 0;

    root = root_obj(r);
    ret = find_map_key(root, key, &val, &f);
    if (ret != MCPKG_MP_NO_ERROR) return ret;
    if (!f) return MCPKG_MP_NO_ERROR;

    if (!val || val->type == MSGPACK_OBJECT_NIL) {
        return MCPKG_MP_NO_ERROR;
    }
    if (val->type != MSGPACK_OBJECT_ARRAY)
        return MCPKG_MP_ERR_PARSE;

    n = val->via.array.size;
    if (n == 0) return MCPKG_MP_NO_ERROR;

    vec = (McPkgDigest *)calloc(n, sizeof(*vec));
    if (!vec) return MCPKG_MP_ERR_NO_MEMORY;

    for (i = 0; i < n; i++) {

        const msgpack_object *elem = &val->via.array.ptr[i];
        if (elem->type != MSGPACK_OBJECT_MAP) {
            ret = MCPKG_MP_ERR_PARSE; break;
        }

        ret = mcpkg_digest_init(&vec[i]);
        if (ret != MCPKG_MP_NO_ERROR)
            break;

        ret = parse_digest_map(elem, &vec[i]);
        if (ret != MCPKG_MP_NO_ERROR)
            break;
    }

    if (ret != MCPKG_MP_NO_ERROR) {
        /* free any constructed digests */
        size_t j;
        for (j = 0; j < i; j++) mcpkg_digest_free(&vec[j]);
        free(vec);
        return ret;
    }

    *out_arr = vec;
    *out_n = n;
    return MCPKG_MP_NO_ERROR;
}
