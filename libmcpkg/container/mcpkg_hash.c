#include "container/mcpkg_hash_p.h"

/* ---------- core table ops (non-inline) ---------- */

static MCPKG_CONTAINER_ERROR rehash(struct McPkgHash *h, size_t new_cap)
{
    char **old_keys;
    uint64_t *old_hashes;
    unsigned char *old_states;
    unsigned char *old_values;
    size_t old_cap, i;
    size_t bytes;
    size_t est;

    char **keys = NULL;
    uint64_t *hashes = NULL;
    unsigned char *states = NULL;
    unsigned char *values = NULL;

    if (!h)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    new_cap = mcpkg_next_pow2_size(new_cap);

    if (table_bytes_est(new_cap, h->value_size, &est))
        return MCPKG_CONTAINER_ERR_OVERFLOW;
    if ((unsigned long long)est > h->max_bytes)
        return MCPKG_CONTAINER_ERR_LIMIT;

    keys = calloc(new_cap, sizeof(*keys));
    hashes = calloc(new_cap, sizeof(*hashes));
    states = calloc(new_cap, sizeof(*states));
    if (!keys || !hashes || !states)
        goto oom;

    if (mcpkg_mul_overflow_size(new_cap, h->value_size, &bytes))
        goto overflow;

    values = malloc(bytes);
    if (!values)
        goto oom;
    memset(values, 0, bytes);

    /* stash old */
    old_keys = h->keys;
    old_hashes = h->hashes;
    old_states = h->states;
    old_values = h->values;
    old_cap = h->cap;

    /* install new */
    h->keys = keys;
    h->hashes = hashes;
    h->states = states;
    h->values = values;
    h->cap = new_cap;
    h->len = 0;

    /* move entries */
    if (old_cap) {
        for (i = 0; i < old_cap; i++) {
            size_t pos, step = 0;

            if (old_states[i] != HST_FULL)
                continue;

            pos = (size_t)(old_hashes[i] & (h->cap - 1));
            while (h->states[pos] == HST_FULL) {
                pos = (pos + 1) & (h->cap - 1);
                if (++step > h->cap)
                    goto corrupt;
            }

            h->keys[pos] = old_keys[i];
            h->hashes[pos] = old_hashes[i];
            h->states[pos] = HST_FULL;

            memcpy(h->values + pos * h->value_size,
                   old_values + i * h->value_size,
                   h->value_size);
            h->len++;
        }

        free(old_keys);
        free(old_hashes);
        free(old_states);
        free(old_values);
    }

    return MCPKG_CONTAINER_OK;

overflow:
    free(keys);
    free(hashes);
    free(states);
    return MCPKG_CONTAINER_ERR_OVERFLOW;

oom:
    free(keys);
    free(hashes);
    free(states);
    free(values);
    return MCPKG_CONTAINER_ERR_NO_MEM;

corrupt:
    /* should never happen; treat as OTHER */
    free(keys);
    free(hashes);
    free(states);
    free(values);
    /* restore old (best effort) */
    h->keys = old_keys;
    h->hashes = old_hashes;
    h->states = old_states;
    h->values = old_values;
    h->cap = old_cap;
    return MCPKG_CONTAINER_ERR_OTHER;
}

static MCPKG_CONTAINER_ERROR maybe_grow(struct McPkgHash *h)
{
    size_t new_cap;

    if (!h)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    if (!h->cap)
        return rehash(h, 8);

    if (!loadfactor_exceeded(h->len + 1, h->cap))
        return MCPKG_CONTAINER_OK;

    new_cap = h->cap << 1;
    return rehash(h, new_cap);
}

/* locate slot for key; on insert path, remember first tombstone */
static int find_slot(const struct McPkgHash *h, const char *key,
                     uint64_t hv, size_t *pos_out, size_t *tomb_out)
{
    size_t pos, step = 0;
    size_t tomb = (size_t)-1;

    if (!h || !key || !pos_out)
        return 0;

    pos = (size_t)(hv & (h->cap - 1));
    for (;;) {
        unsigned char st = h->states[pos];

        if (st == HST_EMPTY) {
            if (tomb_out)
                *tomb_out = tomb;
            *pos_out = pos;
            return 2; /* empty slot, not found */
        }
        if (st == HST_TOMB) {
            if (tomb == (size_t)-1)
                tomb = pos;
        } else {
            if (h->hashes[pos] == hv &&
                h->keys[pos] &&
                strcmp(h->keys[pos], key) == 0) {
                *pos_out = pos;
                if (tomb_out)
                    *tomb_out = tomb;
                return 1; /* found */
            }
        }

        pos = (pos + 1) & (h->cap - 1);
        if (++step > MCPKG_HASH_MAX_PROBE)
            return -1; /* probe bound hit */
    }
}

/* ---------- public API ---------- */

McPkgHash *mcpkg_hash_new(size_t value_size, const McPkgHashOps *ops_or_null,
                          size_t max_pairs, unsigned long long max_bytes)
{
    struct McPkgHash *h;
    size_t est;

    if (!value_size)
        return NULL;

    h = calloc(1, sizeof(*h));
    if (!h)
        return NULL;

    h->value_size = value_size;

    if (ops_or_null)
        h->ops = *ops_or_null;
    else
        memset(&h->ops, 0, sizeof(h->ops));

    h->max_pairs = max_pairs ? max_pairs : MCPKG_CONTAINER_MAX_ELEMENTS;
    h->max_bytes = max_bytes ? max_bytes : MCPKG_CONTAINER_MAX_BYTES;

    /* sanity: minimum table of 8 must fit the byte cap */
    if (table_bytes_est(8, h->value_size, &est) ||
        (unsigned long long)est > h->max_bytes) {
        free(h);
        return NULL;
    }

    mcpkg_sip_seed(&h->k0, &h->k1);
    return h;
}

void mcpkg_hash_free(McPkgHash *h)
{
    size_t i;

    if (!h)
        return;

    if (h->cap && h->keys && h->states) {
        for (i = 0; i < h->cap; i++) {
            if (h->states[i] == HST_FULL && h->keys[i]) {
                free(h->keys[i]);
                h->keys[i] = NULL;
            }
        }
    }

    if (h->ops.value_dtor && h->values && h->cap) {
        for (i = 0; i < h->cap; i++) {
            if (h->states[i] == HST_FULL) {
                void *v = h->values + i * h->value_size;
                h->ops.value_dtor(v, h->ops.ctx);
            }
        }
    }

    free(h->keys);
    free(h->hashes);
    free(h->states);
    free(h->values);
    free(h);
}

size_t mcpkg_hash_size(const McPkgHash *h)
{
    return h ? h->len : 0;
}

void mcpkg_hash_get_limits(const McPkgHash *h, size_t *max_pairs,
                           unsigned long long *max_bytes)
{
    if (!h) {
        if (max_pairs)
            *max_pairs = 0;
        if (max_bytes)
            *max_bytes = 0;
        return;
    }
    if (max_pairs)
        *max_pairs = h->max_pairs;
    if (max_bytes)
        *max_bytes = h->max_bytes;
}

MCPKG_CONTAINER_ERROR
mcpkg_hash_set_limits(McPkgHash *h, size_t max_pairs,
                      unsigned long long max_bytes)
{
    size_t new_pairs, est;

    if (!h)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    new_pairs = max_pairs ? max_pairs : h->max_pairs;
    max_bytes = max_bytes ? max_bytes : h->max_bytes;

    if (h->len > new_pairs)
        return MCPKG_CONTAINER_ERR_LIMIT;

    if (h->cap) {
        if (table_bytes_est(h->cap, h->value_size, &est))
            return MCPKG_CONTAINER_ERR_OVERFLOW;
        if ((unsigned long long)est > max_bytes)
            return MCPKG_CONTAINER_ERR_LIMIT;
    }

    if (table_bytes_est(8, h->value_size, &est))
        return MCPKG_CONTAINER_ERR_OVERFLOW;
    if ((unsigned long long)est > max_bytes)
        return MCPKG_CONTAINER_ERR_LIMIT;

    h->max_pairs = new_pairs;
    h->max_bytes = max_bytes;
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_hash_remove_all(McPkgHash *h)
{
    size_t i;

    if (!h)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    if (h->cap == 0)
        return MCPKG_CONTAINER_OK;

    if (h->keys) {
        for (i = 0; i < h->cap; i++) {
            if (h->states[i] == HST_FULL && h->keys[i]) {
                free(h->keys[i]);
                h->keys[i] = NULL;
            }
        }
    }

    if (h->ops.value_dtor && h->values) {
        for (i = 0; i < h->cap; i++) {
            if (h->states[i] == HST_FULL) {
                void *v = h->values + i * h->value_size;
                h->ops.value_dtor(v, h->ops.ctx);
            }
        }
    }

    memset(h->states, 0, h->cap * sizeof(h->states[0]));
    h->len = 0;
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_hash_remove(McPkgHash *h, const char *key)
{
    uint64_t hv;
    size_t pos;
    int r;

    if (!h || !key)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;
    if (!h->cap || h->len == 0)
        return MCPKG_CONTAINER_ERR_NOT_FOUND;

    hv = key_hash(h, key);

    r = find_slot(h, key, hv, &pos, NULL);
    if (r != 1)
        return MCPKG_CONTAINER_ERR_NOT_FOUND;

    if (h->keys[pos]) {
        free(h->keys[pos]);
        h->keys[pos] = NULL;
    }
    if (h->ops.value_dtor) {
        void *v = h->values + pos * h->value_size;
        h->ops.value_dtor(v, h->ops.ctx);
    }
    h->states[pos] = HST_TOMB;
    h->len--;
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_hash_set(McPkgHash *h, const char *key,
                                     const void *value)
{
    MCPKG_CONTAINER_ERROR ret;
    uint64_t hv;
    size_t pos, tomb = (size_t)-1;
    int r;
    char *kcopy;

    if (!h || !key || !value)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    if (h->len >= h->max_pairs)
        return MCPKG_CONTAINER_ERR_LIMIT;

    ret = maybe_grow(h);
    if (ret != MCPKG_CONTAINER_OK)
        return ret;

    hv = key_hash(h, key);
    r = find_slot(h, key, hv, &pos, &tomb);

    if (r == -1) {
        ret = rehash(h, h->cap << 1);
        if (ret != MCPKG_CONTAINER_OK)
            return ret;

        r = find_slot(h, key, hv, &pos, &tomb);
        if (r == -1)
            return MCPKG_CONTAINER_ERR_LIMIT;
    }

    if (r == 1) {
        if (h->ops.value_copy) {
            h->ops.value_copy(h->values + pos * h->value_size,
                              value, h->ops.ctx);
        } else {
            memcpy(h->values + pos * h->value_size, value,
                   h->value_size);
        }
        return MCPKG_CONTAINER_OK;
    }

    pos = (tomb != (size_t)-1) ? tomb : pos;

    kcopy = dup_cstr(key);
    if (!kcopy)
        return MCPKG_CONTAINER_ERR_NO_MEM;

    h->keys[pos] = kcopy;
    h->hashes[pos] = hv;
    h->states[pos] = HST_FULL;

    if (h->ops.value_copy) {
        h->ops.value_copy(h->values + pos * h->value_size, value,
                          h->ops.ctx);
    } else {
        memcpy(h->values + pos * h->value_size, value,
               h->value_size);
    }

    h->len++;
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_hash_get(const McPkgHash *h, const char *key,
                                     void *out)
{
    uint64_t hv;
    size_t pos;
    int r;

    if (!h || !key || !out)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;
    if (!h->cap || h->len == 0)
        return MCPKG_CONTAINER_ERR_NOT_FOUND;

    hv = key_hash(h, key);
    r = find_slot(h, key, hv, &pos, NULL);
    if (r != 1)
        return MCPKG_CONTAINER_ERR_NOT_FOUND;

    memcpy(out, h->values + pos * h->value_size, h->value_size);
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_hash_pop(McPkgHash *h, const char *key, void *out)
{
    uint64_t hv;
    size_t pos;
    int r;

    if (!h || !key)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;
    if (!h->cap || h->len == 0)
        return MCPKG_CONTAINER_ERR_NOT_FOUND;

    hv = key_hash(h, key);
    r = find_slot(h, key, hv, &pos, NULL);
    if (r != 1)
        return MCPKG_CONTAINER_ERR_NOT_FOUND;

    if (out)
        memcpy(out, h->values + pos * h->value_size, h->value_size);

    if (h->keys[pos]) {
        free(h->keys[pos]);
        h->keys[pos] = NULL;
    }
    if (h->ops.value_dtor) {
        void *v = h->values + pos * h->value_size;
        h->ops.value_dtor(v, h->ops.ctx);
    }
    h->states[pos] = HST_TOMB;
    h->len--;
    return MCPKG_CONTAINER_OK;
}

int mcpkg_hash_contains(const McPkgHash *h, const char *key)
{
    uint64_t hv;
    size_t pos;
    int r;

    if (!h || !key || !h->cap || !h->len)
        return 0;

    hv = key_hash(h, key);
    r = find_slot(h, key, hv, &pos, NULL);
    return r == 1 ? 1 : 0;
}

MCPKG_CONTAINER_ERROR mcpkg_hash_iter_begin(const McPkgHash *h, size_t *it)
{
    if (!h || !it)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;
    *it = 0;
    return MCPKG_CONTAINER_OK;
}

int mcpkg_hash_iter_next(const McPkgHash *h, size_t *it,
                         const char **key_out, void *value_out)
{
    size_t i;

    if (!h || !it)
        return 0;

    for (i = *it; i < h->cap; i++) {
        if (h->states[i] == HST_FULL) {
            if (key_out)
                *key_out = h->keys[i];
            if (value_out)
                memcpy(value_out,
                       h->values + i * h->value_size,
                       h->value_size);
            *it = i + 1;
            return 1;
        }
    }
    *it = h->cap;
    return 0;
}
