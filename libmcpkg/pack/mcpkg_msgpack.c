#include "mcpkg_msgpack.h"

#include <stdlib.h>
#include <string.h>
#include <msgpack.h>


// Common MessagePack helpers for libmcpkg.
// Compact int-keyed maps with tag/version header,
// single-return error handling, no compression here.


// Optional: string list helpers (adjust includes if your path differs)
#include "container/mcpkg_str_list.h"

// -------------------------
// Internal writer/reader
// -------------------------

struct mcpkg_mp_wr {
    msgpack_sbuffer	sbuf;
    msgpack_packer	pk;
    int		init;
};

struct mcpkg_mp_rd {
    msgpack_unpacked	upk;
    msgpack_object		root;
    int			has_root;
    const char		*buf;
    size_t			len;
};

// -------------------------
// Writer API
// -------------------------

int mcpkg_mp_writer_init(struct McPkgMpWriter *w)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;

    if (!w) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }

    wr = (struct mcpkg_mp_wr *)malloc(sizeof(*wr));
    if (!wr) {
        ret = MCPKG_MP_ERR_NO_MEMORY;
        goto out;
    }

    msgpack_sbuffer_init(&wr->sbuf);
    msgpack_packer_init(&wr->pk, &wr->sbuf, msgpack_sbuffer_write);
    wr->init = 1;

    w->impl = wr;
    w->buf = NULL;
    w->len = 0;

out:
    return ret;
}

void mcpkg_mp_writer_destroy(struct McPkgMpWriter *w)
{
    struct mcpkg_mp_wr *wr;

    if (!w || !w->impl)
        return;

    wr = (struct mcpkg_mp_wr *)w->impl;

    if (wr->init)
        msgpack_sbuffer_destroy(&wr->sbuf);

    free(wr);
    w->impl = NULL;

    // If the caller never called finish(), w->buf/w->len are untouched here.
    // If they did, they own w->buf and must free it.
}

int mcpkg_mp_writer_finish(struct McPkgMpWriter *w, void **out_buf,
                           size_t *out_len)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;
    void *dst;

    if (!w || !w->impl || !out_buf || !out_len) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }

    wr = (struct mcpkg_mp_wr *)w->impl;

    dst = malloc(wr->sbuf.size);
    if (!dst) {
        ret = MCPKG_MP_ERR_NO_MEMORY;
        goto out;
    }

    memcpy(dst, wr->sbuf.data, wr->sbuf.size);
    *out_buf = dst;
    *out_len = wr->sbuf.size;

    w->buf = dst;
    w->len = wr->sbuf.size;

out:
    return ret;
}

int mcpkg_mp_map_begin(struct McPkgMpWriter *w, uint32_t key_count)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;

    if (!w || !w->impl) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }

    wr = (struct mcpkg_mp_wr *)w->impl;

    if (msgpack_pack_map(&wr->pk, key_count) != 0) {
        ret = MCPKG_MP_ERR_IO;
        goto out;
    }

out:
    return ret;
}

static int mcpkg__kv_key(struct mcpkg_mp_wr *wr, int key)
{
    if (msgpack_pack_int(&wr->pk, key) != 0)
        return MCPKG_MP_ERR_IO;
    return MCPKG_MP_NO_ERROR;
}

static int mcpkg__pack_str(msgpack_packer *pk, const char *s)
{
    size_t n;

    if (!s) {
        if (msgpack_pack_nil(pk) != 0)
            return MCPKG_MP_ERR_IO;
        return MCPKG_MP_NO_ERROR;
    }

    n = strlen(s);
    if (msgpack_pack_str(pk, (uint32_t)n) != 0)
        return MCPKG_MP_ERR_IO;
    if (n && msgpack_pack_str_body(pk, s, n) != 0)
        return MCPKG_MP_ERR_IO;

    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_kv_i32(struct McPkgMpWriter *w, int key, int32_t v)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;

    if (!w || !w->impl) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }
    wr = (struct mcpkg_mp_wr *)w->impl;

    ret = mcpkg__kv_key(wr, key);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

    if (msgpack_pack_int(&wr->pk, (int)v) != 0) {
        ret = MCPKG_MP_ERR_IO;
        goto out;
    }

out:
    return ret;
}

int mcpkg_mp_kv_u32(struct McPkgMpWriter *w, int key, uint32_t v)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;

    if (!w || !w->impl) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }
    wr = (struct mcpkg_mp_wr *)w->impl;

    ret = mcpkg__kv_key(wr, key);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

    if (msgpack_pack_uint32(&wr->pk, v) != 0) {
        ret = MCPKG_MP_ERR_IO;
        goto out;
    }

out:
    return ret;
}

int mcpkg_mp_kv_i64(struct McPkgMpWriter *w, int key, int64_t v)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;

    if (!w || !w->impl) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }
    wr = (struct mcpkg_mp_wr *)w->impl;

    ret = mcpkg__kv_key(wr, key);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

    if (msgpack_pack_int64(&wr->pk, v) != 0) {
        ret = MCPKG_MP_ERR_IO;
        goto out;
    }

out:
    return ret;
}

int mcpkg_mp_kv_str(struct McPkgMpWriter *w, int key, const char *s)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;

    if (!w || !w->impl) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }
    wr = (struct mcpkg_mp_wr *)w->impl;

    ret = mcpkg__kv_key(wr, key);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

    ret = mcpkg__pack_str(&wr->pk, s);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

out:
    return ret;
}

int mcpkg_mp_kv_bin(struct McPkgMpWriter *w, int key,
                    const void *data, uint32_t len)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;

    if (!w || !w->impl) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }
    wr = (struct mcpkg_mp_wr *)w->impl;

    ret = mcpkg__kv_key(wr, key);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

    if (data && len) {
        if (msgpack_pack_bin(&wr->pk, len) != 0) {
            ret = MCPKG_MP_ERR_IO;
            goto out;
        }
        if (msgpack_pack_bin_body(&wr->pk, data, len) != 0) {
            ret = MCPKG_MP_ERR_IO;
            goto out;
        }
    } else {
        if (msgpack_pack_bin(&wr->pk, 0) != 0) {
            ret = MCPKG_MP_ERR_IO;
            goto out;
        }
    }

out:
    return ret;
}

int mcpkg_mp_kv_nil(struct McPkgMpWriter *w, int key)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;

    if (!w || !w->impl) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }
    wr = (struct mcpkg_mp_wr *)w->impl;

    ret = mcpkg__kv_key(wr, key);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

    if (msgpack_pack_nil(&wr->pk) != 0) {
        ret = MCPKG_MP_ERR_IO;
        goto out;
    }

out:
    return ret;
}

int mcpkg_mp_write_header(struct McPkgMpWriter *w,
                          const char *tag, int version)
{
    int ret = MCPKG_MP_NO_ERROR;

    if (!w || !tag) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }

    ret = mcpkg_mp_kv_str(w, MCPKG_MP_K_TAG, tag);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

    ret = mcpkg_mp_kv_i32(w, MCPKG_MP_K_VER, version);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

out:
    return ret;
}

// -------------------------
// Reader API
// -------------------------

int mcpkg_mp_reader_init(struct McPkgMpReader *r,
                         const void *buf, size_t len)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_rd *rd;

    if (!r || !buf || !len) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }

    rd = (struct mcpkg_mp_rd *)malloc(sizeof(*rd));
    if (!rd) {
        ret = MCPKG_MP_ERR_NO_MEMORY;
        goto out;
    }

    msgpack_unpacked_init(&rd->upk);
    if (!msgpack_unpack_next(&rd->upk, (const char *)buf, len, NULL)) {
        msgpack_unpacked_destroy(&rd->upk);
        free(rd);
        ret = MCPKG_MP_ERR_PARSE;
        goto out;
    }

    rd->root = rd->upk.data;
    if (rd->root.type != MSGPACK_OBJECT_MAP) {
        msgpack_unpacked_destroy(&rd->upk);
        free(rd);
        ret = MCPKG_MP_ERR_PARSE;
        goto out;
    }

    rd->has_root = 1;
    rd->buf = (const char *)buf;
    rd->len = len;

    r->impl = rd;
    r->buf = buf;
    r->len = len;
    r->root = &rd->root;

out:
    return ret;
}

void mcpkg_mp_reader_destroy(struct McPkgMpReader *r)
{
    struct mcpkg_mp_rd *rd;

    if (!r || !r->impl)
        return;

    rd = (struct mcpkg_mp_rd *)r->impl;

    if (rd->has_root)
        msgpack_unpacked_destroy(&rd->upk);

    free(rd);
    r->impl = NULL;
    r->root = NULL;
    r->buf = NULL;
    r->len = 0;
}

static int mcpkg__find_map_key(const struct mcpkg_mp_rd *rd, int key,
                               msgpack_object *out_val, int *found)
{
    size_t i, n;

    if (!rd || !out_val || !found)
        return MCPKG_MP_ERR_INVALID_ARG;

    *found = 0;

    if (!rd->has_root || rd->root.type != MSGPACK_OBJECT_MAP)
        return MCPKG_MP_ERR_PARSE;

    n = rd->root.via.map.size;
    for (i = 0; i < n; i++) {
        msgpack_object k = rd->root.via.map.ptr[i].key;
        msgpack_object v = rd->root.via.map.ptr[i].val;

        if (k.type != MSGPACK_OBJECT_POSITIVE_INTEGER &&
            k.type != MSGPACK_OBJECT_NEGATIVE_INTEGER)
            continue;

        if ((int)k.via.i64 == key) {
            *out_val = v;
            *found = 1;
            return MCPKG_MP_NO_ERROR;
        }
    }

    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_expect_tag(const struct McPkgMpReader *r,
                        const char *expected_tag, int *out_version)
{
    int ret = MCPKG_MP_NO_ERROR;
    const struct mcpkg_mp_rd *rd;
    msgpack_object v;
    int found = 0;

    if (!r || !r->impl || !expected_tag) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }

    rd = (const struct mcpkg_mp_rd *)r->impl;

    ret = mcpkg__find_map_key(rd, MCPKG_MP_K_TAG, &v, &found);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;
    if (!found || v.type != MSGPACK_OBJECT_STR) {
        ret = MCPKG_MP_ERR_PARSE;
        goto out;
    }
    if (v.via.str.size != strlen(expected_tag) ||
        memcmp(v.via.str.ptr, expected_tag, v.via.str.size) != 0) {
        ret = MCPKG_MP_ERR_PARSE;
        goto out;
    }

    if (out_version) {
        found = 0;
        ret = mcpkg__find_map_key(rd, MCPKG_MP_K_VER, &v, &found);
        if (ret != MCPKG_MP_NO_ERROR)
            goto out;
        if (found &&
            (v.type == MSGPACK_OBJECT_POSITIVE_INTEGER ||
             v.type == MSGPACK_OBJECT_NEGATIVE_INTEGER)) {
            *out_version = (int)v.via.i64;
        } else {
            *out_version = 1; // default if absent
        }
    }

out:
    return ret;
}

int mcpkg_mp_get_i64(const struct McPkgMpReader *r, int key,
                     int64_t *out, int *found)
{
    int ret = MCPKG_MP_NO_ERROR;
    const struct mcpkg_mp_rd *rd;
    msgpack_object v;
    int f = 0;

    if (!r || !r->impl || !out)
        return MCPKG_MP_ERR_INVALID_ARG;

    rd = (const struct mcpkg_mp_rd *)r->impl;

    ret = mcpkg__find_map_key(rd, key, &v, &f);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    if (!f) {
        if (found) *found = 0;
        return MCPKG_MP_NO_ERROR;
    }

    if (v.type != MSGPACK_OBJECT_POSITIVE_INTEGER &&
        v.type != MSGPACK_OBJECT_NEGATIVE_INTEGER)
        return MCPKG_MP_ERR_PARSE;

    *out = v.via.i64;
    if (found) *found = 1;
    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_get_u64(const struct McPkgMpReader *r, int key,
                     uint64_t *out, int *found)
{
    int ret = MCPKG_MP_NO_ERROR;
    int64_t tmp;
    int f = 0;

    if (!out)
        return MCPKG_MP_ERR_INVALID_ARG;

    ret = mcpkg_mp_get_i64(r, key, &tmp, &f);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    if (!f) {
        if (found) *found = 0;
        return MCPKG_MP_NO_ERROR;
    }
    if (tmp < 0)
        return MCPKG_MP_ERR_PARSE;

    *out = (uint64_t)tmp;
    if (found) *found = 1;
    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_get_u32(const struct McPkgMpReader *r, int key,
                     uint32_t *out, int *found)
{
    int ret = MCPKG_MP_NO_ERROR;
    uint64_t u64;
    int f = 0;

    if (!out)
        return MCPKG_MP_ERR_INVALID_ARG;

    ret = mcpkg_mp_get_u64(r, key, &u64, &f);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    if (!f) {
        if (found) *found = 0;
        return MCPKG_MP_NO_ERROR;
    }
    if (u64 > 0xffffffffULL)
        return MCPKG_MP_ERR_PARSE;

    *out = (uint32_t)u64;
    if (found) *found = 1;
    return MCPKG_MP_NO_ERROR;
}

int mcpkg_mp_get_str_borrow(const struct McPkgMpReader *r, int key,
                            const char **out_ptr, size_t *out_len, int *found)
{
    int ret = MCPKG_MP_NO_ERROR;
    const struct mcpkg_mp_rd *rd;
    msgpack_object v;
    int f = 0;

    if (!r || !r->impl || !out_ptr || !out_len)
        return MCPKG_MP_ERR_INVALID_ARG;

    rd = (const struct mcpkg_mp_rd *)r->impl;

    ret = mcpkg__find_map_key(rd, key, &v, &f);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    if (!f) {
        *out_ptr = NULL;
        *out_len = 0;
        if (found) *found = 0;
        return MCPKG_MP_NO_ERROR;
    }

    if (v.type == MSGPACK_OBJECT_STR) {
        *out_ptr = v.via.str.ptr;
        *out_len = v.via.str.size;
        if (found) *found = 1;
        return MCPKG_MP_NO_ERROR;
    }

    if (v.type == MSGPACK_OBJECT_NIL) {
        *out_ptr = NULL;
        *out_len = 0;
        if (found) *found = 1;
        return MCPKG_MP_NO_ERROR;
    }

    return MCPKG_MP_ERR_PARSE;
}

int mcpkg_mp_get_bin_borrow(const struct McPkgMpReader *r, int key,
                            const void **out_ptr, size_t *out_len, int *found)
{
    int ret = MCPKG_MP_NO_ERROR;
    const struct mcpkg_mp_rd *rd;
    msgpack_object v;
    int f = 0;

    if (!r || !r->impl || !out_ptr || !out_len)
        return MCPKG_MP_ERR_INVALID_ARG;

    rd = (const struct mcpkg_mp_rd *)r->impl;

    ret = mcpkg__find_map_key(rd, key, &v, &f);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    if (!f) {
        *out_ptr = NULL;
        *out_len = 0;
        if (found) *found = 0;
        return MCPKG_MP_NO_ERROR;
    }

    if (v.type == MSGPACK_OBJECT_BIN) {
        *out_ptr = v.via.bin.ptr;
        *out_len = v.via.bin.size;
        if (found) *found = 1;
        return MCPKG_MP_NO_ERROR;
    }

    if (v.type == MSGPACK_OBJECT_NIL) {
        *out_ptr = NULL;
        *out_len = 0;
        if (found) *found = 1;
        return MCPKG_MP_NO_ERROR;
    }

    return MCPKG_MP_ERR_PARSE;
}

// -------------------------
// String list helpers
// -------------------------

int mcpkg_mp_kv_strlist(struct McPkgMpWriter *w, int key,
                        const McPkgStringList *sl)
{
    int ret = MCPKG_MP_NO_ERROR;
    struct mcpkg_mp_wr *wr;
    size_t i, n;

    if (!w || !w->impl) {
        ret = MCPKG_MP_ERR_INVALID_ARG;
        goto out;
    }
    wr = (struct mcpkg_mp_wr *)w->impl;

    ret = mcpkg__kv_key(wr, key);
    if (ret != MCPKG_MP_NO_ERROR)
        goto out;

    if (!sl) {
        if (msgpack_pack_nil(&wr->pk) != 0) {
            ret = MCPKG_MP_ERR_IO;
            goto out;
        }
        goto out;
    }

    n = mcpkg_stringlist_size(sl);
    if (msgpack_pack_array(&wr->pk, (uint32_t)n) != 0) {
        ret = MCPKG_MP_ERR_IO;
        goto out;
    }

    for (i = 0; i < n; i++) {
        const char *s = mcpkg_stringlist_at(sl, i);
        ret = mcpkg__pack_str(&wr->pk, s ? s : "");
        if (ret != MCPKG_MP_NO_ERROR)
            goto out;
    }

out:
    return ret;
}

int mcpkg_mp_get_strlist_dup(const struct McPkgMpReader *r, int key,
                             McPkgStringList **out_sl)
{
    int ret = MCPKG_MP_NO_ERROR;
    const struct mcpkg_mp_rd *rd;
    msgpack_object v;
    int found = 0;
    McPkgStringList *sl = NULL;
    size_t i, n;

    if (!r || !r->impl || !out_sl)
        return MCPKG_MP_ERR_INVALID_ARG;

    *out_sl = NULL;
    rd = (const struct mcpkg_mp_rd *)r->impl;

    ret = mcpkg__find_map_key(rd, key, &v, &found);
    if (ret != MCPKG_MP_NO_ERROR)
        return ret;

    if (!found || v.type == MSGPACK_OBJECT_NIL) {
        // Missing or nil -> leave *out_sl NULL
        return MCPKG_MP_NO_ERROR;
    }

    if (v.type != MSGPACK_OBJECT_ARRAY)
        return MCPKG_MP_ERR_PARSE;

    n = v.via.array.size;
    sl = mcpkg_stringlist_new(0, 0); // no hard caps by default
    if (!sl)
        return MCPKG_MP_ERR_NO_MEMORY;

    for (i = 0; i < n; i++) {
        msgpack_object e = v.via.array.ptr[i];

        if (e.type != MSGPACK_OBJECT_STR) {
            ret = MCPKG_MP_ERR_PARSE;
            goto out_free;
        }
        {
            char *dup = strndup(e.via.str.ptr, e.via.str.size);
            if (!dup) {
                ret = MCPKG_MP_ERR_NO_MEMORY;
                goto out_free;
            }
            if (mcpkg_stringlist_push(sl, dup) != MCPKG_CONTAINER_OK) {
                free(dup);
                ret = MCPKG_MP_ERR_NO_MEMORY;
                goto out_free;
            }
            free(dup); // push() duplicates, so free our temp
        }
    }

    *out_sl = sl;
    return MCPKG_MP_NO_ERROR;

out_free:
    mcpkg_stringlist_free(sl);
    return ret;
}

