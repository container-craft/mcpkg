#ifndef MCPKG_MAP_P_H
#define MCPKG_MAP_P_H

#include <stdlib.h>
#include <string.h>

#include "container/mcpkg_map.h"      /* public API (opaque type) */
#include "math/mcpkg_math.h"          /* overflow helpers */

/*
 * McPkgMap internals (RB-tree).
 * - Include ONLY from mcpkg_map.c (not installed).
 * - Keep helpers tiny & static inline.
 */

/* ----- internal node ----- */

struct rb_node {
    struct rb_node *left;
    struct rb_node *right;
    struct rb_node *parent;
    unsigned char  color;   /* 0=black, 1=red */
    char          *key;     /* owned C string */
    unsigned char  value[]; /* flexible array: value_size bytes */
};

/* full definition of the opaque map */
struct McPkgMap {
    struct rb_node        *root;
    size_t                 size;

    size_t                 value_size;
    McPkgMapOps            ops;

    size_t                 max_pairs;
    unsigned long long     max_bytes;

    /* accounting for nodes only (sizeof(node)+value_size per entry) */
    unsigned long long     bytes_used;
};

/* ----- tiny utils (inline) ----- */

static inline unsigned char node_color(const struct rb_node *n)
{
    return n ? n->color : 0;
}

static inline void set_color(struct rb_node *n, unsigned char c)
{
    if (n)
        n->color = c;
}

static inline size_t node_bytes(const struct McPkgMap *m)
{
    return sizeof(struct rb_node) + m->value_size;
}

static inline int under_byte_cap(const struct McPkgMap *m, size_t add_nodes)
{
    uint64_t cur = (uint64_t)m->bytes_used;
    uint64_t add = (uint64_t)node_bytes(m) * (uint64_t)add_nodes;
    uint64_t need;

    if (mcpkg_add_overflow_u64(cur, add, &need))
        return 0;
    return need <= m->max_bytes;
}

static inline char *dup_cstr(const char *s)
{
    size_t n;
    char *p;

    if (!s)
        return NULL;
    n = strlen(s);
    p = malloc(n + 1);
    if (!p)
        return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static inline struct rb_node *tree_min(struct rb_node *x)
{
    while (x && x->left)
        x = x->left;
    return x;
}

static inline struct rb_node *successor(struct rb_node *x)
{
    if (!x)
        return NULL;
    if (x->right)
        return tree_min(x->right);
    while (x->parent && x == x->parent->right)
        x = x->parent;
    return x->parent;
}

/* Rotations are small; inline is fine. */

static inline void rotate_left(struct McPkgMap *m, struct rb_node *x)
{
    struct rb_node *y = x->right;

    x->right = y->left;
    if (y->left)
        y->left->parent = x;

    y->parent = x->parent;
    if (!x->parent)
        m->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->left = x;
    x->parent = y;
}

static inline void rotate_right(struct McPkgMap *m, struct rb_node *x)
{
    struct rb_node *y = x->left;

    x->left = y->right;
    if (y->right)
        y->right->parent = x;

    y->parent = x->parent;
    if (!x->parent)
        m->root = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;

    y->right = x;
    x->parent = y;
}

#endif /* MCPKG_MAP_P_H */
