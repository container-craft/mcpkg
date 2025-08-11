#ifndef MCPKG_VISITED_SET_H
#define MCPKG_VISITED_SET_H

#include "utils/mcpkg_export.h"
#include <stdlib.h>
#include <string.h>
MCPKG_BEGIN_DECLS

typedef struct {
    char **ids;
    size_t count, cap;
} VisitedSet;

static void mcpkg_visited_free(VisitedSet *vs)
{
    if (!vs)
        return;
    for (size_t i = 0; i < vs->count; ++i)
        free(vs->ids[i]);

    free(vs->ids);
    memset(vs, 0, sizeof(*vs));
}

static int mcpkg_visited_contains(VisitedSet *vs,
                                      const char *id)
{
    if (!vs || !id)
        return 0;

    for (size_t i = 0; i < vs->count; ++i) {
        if (vs->ids[i] && strcmp(vs->ids[i], id) == 0)
            return 1;
    }
    return 0;
}

static int mcpkg_visited_add(VisitedSet *vs,
                                 const char *id)
{
    if (!vs || !id)
        return 1;
    if (mcpkg_visited_contains(vs, id))
        return 0;
    if (vs->count == vs->cap) {
        size_t ncap = vs->cap ? vs->cap * 2 : 8;
        char **tmp = realloc(vs->ids, ncap * sizeof(*tmp));
        if (!tmp)
            return 1;
        vs->ids = tmp; vs->cap = ncap;
    }
    vs->ids[vs->count++] = strdup(id);
    return 0;
}
MCPKG_END_DECLS
#endif // MCPKG_VISITED_SET_H
