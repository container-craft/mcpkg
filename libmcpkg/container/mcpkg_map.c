#include "mcpkg_map.h"
#include "mcpkg_map_p.h"

/* ----- search helpers (non-inline) ----- */

static struct rb_node *find_node(const McPkgMap *m, const char *key)
{
    struct rb_node *cur = m->root;
    int cmp;

    while (cur) {
        cmp = strcmp(key, cur->key);
        if (cmp == 0)
            return cur;
        cur = (cmp < 0) ? cur->left : cur->right;
    }
    return NULL;
}

static struct rb_node *lower_bound_node(const McPkgMap *m, const char *key)
{
    struct rb_node *cur = m->root, *res = NULL;
    int cmp;

    while (cur) {
        cmp = strcmp(key, cur->key);
        if (cmp <= 0) {
            res = cur;
            cur = cur->left;
        } else {
            cur = cur->right;
        }
    }
    return res;
}

/* ----- rotations already inline in _p.h ----- */

/* ----- transplant & fixups ----- */

static void transplant(McPkgMap *m, struct rb_node *u, struct rb_node *v)
{
    if (!u->parent)
        m->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;

    if (v)
        v->parent = u->parent;
}

static void insert_fixup(McPkgMap *m, struct rb_node *z)
{
    while (node_color(z->parent) == 1) {
        if (z->parent == z->parent->parent->left) {
            struct rb_node *y = z->parent->parent->right;

            if (node_color(y) == 1) {
                set_color(z->parent, 0);
                set_color(y, 0);
                set_color(z->parent->parent, 1);
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rotate_left(m, z);
                }
                set_color(z->parent, 0);
                set_color(z->parent->parent, 1);
                rotate_right(m, z->parent->parent);
            }
        } else {
            struct rb_node *y = z->parent->parent->left;

            if (node_color(y) == 1) {
                set_color(z->parent, 0);
                set_color(y, 0);
                set_color(z->parent->parent, 1);
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rotate_right(m, z);
                }
                set_color(z->parent, 0);
                set_color(z->parent->parent, 1);
                rotate_left(m, z->parent->parent);
            }
        }
    }
    set_color(m->root, 0);
}

static void delete_fixup(McPkgMap *m, struct rb_node *x,
                         struct rb_node *x_parent)
{
    while ((x != m->root) && (node_color(x) == 0)) {
        if (x == (x_parent ? x_parent->left : NULL)) {
            struct rb_node *w = x_parent ? x_parent->right : NULL;

            if (node_color(w) == 1) {
                set_color(w, 0);
                set_color(x_parent, 1);
                rotate_left(m, x_parent);
                w = x_parent ? x_parent->right : NULL;
            }
            if (node_color(w ? w->left : NULL) == 0 &&
                node_color(w ? w->right : NULL) == 0) {
                set_color(w, 1);
                x = x_parent;
                x_parent = x ? x->parent : NULL;
            } else {
                if (node_color(w ? w->right : NULL) == 0) {
                    set_color(w ? w->left : NULL, 0);
                    set_color(w, 1);
                    rotate_right(m, w);
                    w = x_parent ? x_parent->right : NULL;
                }
                set_color(w, node_color(x_parent));
                set_color(x_parent, 0);
                set_color(w ? w->right : NULL, 0);
                rotate_left(m, x_parent);
                x = m->root;
                break;
            }
        } else {
            struct rb_node *w = x_parent ? x_parent->left : NULL;

            if (node_color(w) == 1) {
                set_color(w, 0);
                set_color(x_parent, 1);
                rotate_right(m, x_parent);
                w = x_parent ? x_parent->left : NULL;
            }
            if (node_color(w ? w->right : NULL) == 0 &&
                node_color(w ? w->left : NULL) == 0) {
                set_color(w, 1);
                x = x_parent;
                x_parent = x ? x->parent : NULL;
            } else {
                if (node_color(w ? w->left : NULL) == 0) {
                    set_color(w ? w->right : NULL, 0);
                    set_color(w, 1);
                    rotate_left(m, w);
                    w = x_parent ? x_parent->left : NULL;
                }
                set_color(w, node_color(x_parent));
                set_color(x_parent, 0);
                set_color(w ? w->left : NULL, 0);
                rotate_right(m, x_parent);
                x = m->root;
                break;
            }
        }
    }
    if (x)
        set_color(x, 0);
}

/* ----- public API ----- */

McPkgMap *mcpkg_map_new(size_t value_size, const McPkgMapOps *ops_or_null,
                        size_t max_pairs, unsigned long long max_bytes)
{
    McPkgMap *m;

    if (!value_size)
        return NULL;

    m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;

    m->value_size = value_size;
    if (ops_or_null)
        m->ops = *ops_or_null;
    else
        memset(&m->ops, 0, sizeof(m->ops));

    m->max_pairs = max_pairs ? max_pairs : MCPKG_CONTAINER_MAX_ELEMENTS;
    m->max_bytes = max_bytes ? max_bytes : MCPKG_CONTAINER_MAX_BYTES;
    m->bytes_used = 0;

    if ((unsigned long long)node_bytes(m) > m->max_bytes) {
        free(m);
        return NULL;
    }

    return m;
}

void mcpkg_map_free(McPkgMap *m)
{
    struct rb_node *n;

    if (!m)
        return;

    /* prune leaves iteratively; never touch freed memory */
    n = m->root;
    while (n) {
        if (n->left) {
            n = n->left;
            continue;
        }
        if (n->right) {
            n = n->right;
            continue;
        }

        /* n is a leaf: detach from parent, then free */
        struct rb_node *parent = n->parent;

        if (m->ops.value_dtor)
            m->ops.value_dtor(n->value, m->ops.ctx);
        if (n->key)
            free(n->key);
        /* detach before free so we never deref a dangling child */
        if (parent) {
            if (parent->left == n)
                parent->left = NULL;
            else if (parent->right == n)
                parent->right = NULL;
        }
        free(n);

        n = parent; /* climb */
    }

    free(m);
}


size_t mcpkg_map_size(const McPkgMap *m)
{
    return m ? m->size : 0;
}

void mcpkg_map_get_limits(const McPkgMap *m, size_t *max_pairs,
                          unsigned long long *max_bytes)
{
    if (!m) {
        if (max_pairs)
            *max_pairs = 0;
        if (max_bytes)
            *max_bytes = 0;
        return;
    }
    if (max_pairs)
        *max_pairs = m->max_pairs;
    if (max_bytes)
        *max_bytes = m->max_bytes;
}

MCPKG_CONTAINER_ERROR
mcpkg_map_set_limits(McPkgMap *m, size_t max_pairs,
                     unsigned long long max_bytes)
{
    if (!m)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    max_pairs = max_pairs ? max_pairs : m->max_pairs;
    max_bytes = max_bytes ? max_bytes : m->max_bytes;

    if (m->size > max_pairs)
        return MCPKG_CONTAINER_ERR_LIMIT;
    if (m->bytes_used > max_bytes)
        return MCPKG_CONTAINER_ERR_LIMIT;

    m->max_pairs = max_pairs;
    m->max_bytes = max_bytes;
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_map_set(McPkgMap *m, const char *key,
                                    const void *value)
{
    struct rb_node *cur, *parent = NULL, *z;
    int cmp;

    if (!m || !key || !value)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    cur = m->root;
    while (cur) {
        cmp = strcmp(key, cur->key);
        if (cmp == 0) {
            if (m->ops.value_copy)
                m->ops.value_copy(cur->value, value,
                                  m->ops.ctx);
            else
                memcpy(cur->value, value, m->value_size);
            return MCPKG_CONTAINER_OK;
        }
        parent = cur;
        cur = (cmp < 0) ? cur->left : cur->right;
    }

    if (m->size >= m->max_pairs)
        return MCPKG_CONTAINER_ERR_LIMIT;
    if (!under_byte_cap(m, 1))
        return MCPKG_CONTAINER_ERR_LIMIT;

    z = malloc(sizeof(*z) + m->value_size);
    if (!z)
        return MCPKG_CONTAINER_ERR_NO_MEM;

    z->left = z->right = z->parent = NULL;
    set_color(z, 1);		/* red */
    z->key = dup_cstr(key);
    if (!z->key) {
        free(z);
        return MCPKG_CONTAINER_ERR_NO_MEM;
    }

    if (m->ops.value_copy)
        m->ops.value_copy(z->value, value, m->ops.ctx);
    else
        memcpy(z->value, value, m->value_size);

    z->parent = parent;
    if (!parent) {
        m->root = z;
    } else if (strcmp(z->key, parent->key) < 0) {
        parent->left = z;
    } else {
        parent->right = z;
    }
    insert_fixup(m, z);

    m->size++;
    m->bytes_used += node_bytes(m);
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_map_get(const McPkgMap *m, const char *key,
                                    void *out)
{
    struct rb_node *n;

    if (!m || !key || !out)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    n = find_node(m, key);
    if (!n)
        return MCPKG_CONTAINER_ERR_NOT_FOUND;

    memcpy(out, n->value, m->value_size);
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_map_remove(McPkgMap *m, const char *key)
{
    struct rb_node *z, *y, *x, *x_parent;
    unsigned char y_color;

    if (!m || !key)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    z = find_node(m, key);
    if (!z)
        return MCPKG_CONTAINER_ERR_NOT_FOUND;

    y = z;
    y_color = node_color(y);

    if (!z->left) {
        x = z->right;
        x_parent = z->parent;
        transplant(m, z, z->right);
    } else if (!z->right) {
        x = z->left;
        x_parent = z->parent;
        transplant(m, z, z->left);
    } else {
        y = tree_min(z->right);
        y_color = node_color(y);
        x = y->right;
        x_parent = y->parent;

        if (y->parent == z) {
            if (x)
                x->parent = y;
            x_parent = y;
        } else {
            transplant(m, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(m, z, y);
        y->left = z->left;
        y->left->parent = y;
        set_color(y, node_color(z));
    }

    if (m->ops.value_dtor)
        m->ops.value_dtor(z->value, m->ops.ctx);
    if (z->key)
        free(z->key);
    free(z);

    m->size--;
    m->bytes_used -= node_bytes(m);

    if (y_color == 0)
        delete_fixup(m, x, x_parent);

    return MCPKG_CONTAINER_OK;
}

int mcpkg_map_contains(const McPkgMap *m, const char *key)
{
    return (m && key && find_node(m, key)) ? 1 : 0;
}

/* ----- ordered iteration ----- */

MCPKG_CONTAINER_ERROR
mcpkg_map_iter_begin(const McPkgMap *m, void **iter)
{
    if (!m || !iter)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    *iter = m->root ? (void *)tree_min(m->root) : NULL;
    return MCPKG_CONTAINER_OK;
}

int mcpkg_map_iter_next(const McPkgMap *m, void **iter,
                        const char **key_out, void *value_out)
{
    struct rb_node *n;

    if (!m || !iter)
        return 0;

    n = (struct rb_node *)(*iter);
    if (!n)
        return 0;

    if (key_out)
        *key_out = n->key;
    if (value_out)
        memcpy(value_out, n->value, m->value_size);

    *iter = (void *)successor(n);
    return 1;
}

MCPKG_CONTAINER_ERROR
mcpkg_map_iter_seek(const McPkgMap *m, void **iter, const char *seek_key)
{
    if (!m || !iter || !seek_key)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;

    *iter = (void *)lower_bound_node(m, seek_key);
    return MCPKG_CONTAINER_OK;
}

/* ----- first/last ----- */

MCPKG_CONTAINER_ERROR
mcpkg_map_first(const McPkgMap *m, const char **key_out, void *value_out)
{
    struct rb_node *n;

    if (!m || !key_out)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;
    if (!m->root)
        return MCPKG_CONTAINER_ERR_RANGE;

    n = tree_min(m->root);
    *key_out = n->key;
    if (value_out)
        memcpy(value_out, n->value, m->value_size);
    return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR
mcpkg_map_last(const McPkgMap *m, const char **key_out, void *value_out)
{
    struct rb_node *n;

    if (!m || !key_out)
        return MCPKG_CONTAINER_ERR_NULL_PARAM;
    if (!m->root)
        return MCPKG_CONTAINER_ERR_RANGE;

    n = m->root;
    while (n->right)
        n = n->right;

    *key_out = n->key;
    if (value_out)
        memcpy(value_out, n->value, m->value_size);
    return MCPKG_CONTAINER_OK;
}
