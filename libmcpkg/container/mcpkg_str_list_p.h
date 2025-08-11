#ifndef MCPKG_STR_LIST_P_H
#define MCPKG_STR_LIST_P_H

#include <stdlib.h>
#include <string.h>

#include "container/mcpkg_str_list.h"     /* public API decls */
#include "container/mcpkg_list.h"         /* base list impl */

/*
 * McPkgStringList internals.
 * Included only by mcpkg_str_list.c (not installed).
 */

struct McPkgStringList {
    McPkgList		*lst;		/* list of (char *) */
};

/* dtor for McPkgList elements (char *). */
static inline void strlist_elem_dtor(void *elem, void *ctx)
{
    char **p = (char **)elem;

    (void)ctx;
    if (p && *p) {
        free(*p);
        *p = NULL;
    }
}

/* build the underlying McPkgList configured for (char *) elements */
static inline McPkgList *strlist_make_base(size_t max_elements,
                                           unsigned long long max_bytes)
{
    McPkgListOps ops;

    memset(&ops, 0, sizeof(ops));
    ops.copy = NULL;			/* caller handles strdup */
    ops.dtor = strlist_elem_dtor;		/* free(char*) on remove */
    ops.equals = NULL;			/* memcmp fallback is fine */
    ops.ctx = NULL;

    return mcpkg_list_new(sizeof(char *), &ops, max_elements, max_bytes);
}

/* borrowed pointer helper; invalid after reallocation/mutation */
static inline const char *sl_at_borrow(const McPkgStringList *sl, size_t i)
{
    const char *const *pp;

    if (!sl)
        return NULL;

    pp = (const char *const *)mcpkg_list_at_cptr(sl->lst, i);
    return pp ? *pp : NULL;
}

#endif /* MCPKG_STR_LIST_P_H */
