// Abstract loader registry and helpers.
#include "mcpkg_mc_loaders.h"

#include <stdlib.h>
#include <string.h>

#include "mc/mcpkg_mc_util.h"

// Static defaults for known loaders. These are templates; caller copies by value.
// base_url may be overridden at runtime. Flags are advisory capabilities.

static const struct McPkgMcLoader g_loader_table[] = {
    {
        MCPKG_MC_LOADER_VANILLA,
        "vanilla",
        NULL,
        MCPKG_MC_LOADER_F_SUPPORTS_CLIENT | MCPKG_MC_LOADER_F_SUPPORTS_SERVER,
        NULL,
        NULL,
        0
    },
    {
        MCPKG_MC_LOADER_FORGE,
        "forge",
        NULL,
        MCPKG_MC_LOADER_F_SUPPORTS_CLIENT | MCPKG_MC_LOADER_F_SUPPORTS_SERVER,
        NULL,
        NULL,
        0
    },
    {
        MCPKG_MC_LOADER_FABRIC,
        "fabric",
        NULL,
        MCPKG_MC_LOADER_F_SUPPORTS_CLIENT | MCPKG_MC_LOADER_F_SUPPORTS_SERVER,
        NULL,
        NULL,
        0
    },
    {
        MCPKG_MC_LOADER_QUILT,
        "quilt",
        NULL,
        MCPKG_MC_LOADER_F_SUPPORTS_CLIENT | MCPKG_MC_LOADER_F_SUPPORTS_SERVER,
        NULL,
        NULL,
        0
    },
    {
        MCPKG_MC_LOADER_PAPER,
        "paper",
        NULL,
        MCPKG_MC_LOADER_F_SUPPORTS_SERVER | MCPKG_MC_LOADER_F_HAS_API,
        NULL,
        NULL,
        0
    },
    {
        MCPKG_MC_LOADER_PURPUR,
        "purpur",
        NULL,
        MCPKG_MC_LOADER_F_SUPPORTS_SERVER,
        NULL,
        NULL,
        0
    },
    {
        MCPKG_MC_LOADER_VELOCITY,
        "velocity",
        NULL,
        MCPKG_MC_LOADER_F_SUPPORTS_SERVER | MCPKG_MC_LOADER_F_HAS_API,
        NULL,
        NULL,
        0
    },
};

static size_t mcpkg_mc_loader_table_count(void)
{
    return sizeof(g_loader_table) / sizeof(g_loader_table[0]);
}

static const struct McPkgMcLoader *
mcpkg_mc_loader_find_by_id(MCPKG_MC_LOADERS id)
{
    size_t i, n = mcpkg_mc_loader_table_count();

    for (i = 0; i < n; i++) {
        if (g_loader_table[i].loader == id)
            return &g_loader_table[i];
    }
    return NULL;
}

static const struct McPkgMcLoader *
mcpkg_mc_loader_find_by_name(const char *s)
{
    size_t i, n;

    if (!s)
        return NULL;

    n = mcpkg_mc_loader_table_count();
    for (i = 0; i < n; i++) {
        if (mcpkg_mc_ascii_ieq(g_loader_table[i].name, s))
            return &g_loader_table[i];
    }
    return NULL;
}

// Heap constructor: outp gets an owned instance initialized from the template.
int mcpkg_mc_loader_new(struct McPkgMcLoader **outp, MCPKG_MC_LOADERS id)
{
    int ret = MCPKG_MC_NO_ERROR;
    const struct McPkgMcLoader *tmpl;
    struct McPkgMcLoader *l;

    if (!outp) {
        ret = MCPKG_MC_ERR_INVALID_ARG;
        goto out;
    }

    tmpl = mcpkg_mc_loader_find_by_id(id);
    if (!tmpl) {
        ret = MCPKG_MC_ERR_NOT_FOUND;
        goto out;
    }

    l = (struct McPkgMcLoader *)malloc(sizeof(*l));
    if (!l) {
        ret = MCPKG_MC_ERR_NO_MEMORY;
        goto out;
    }

    *l = *tmpl;
    l->owns_base_url = 0;
    l->priv = NULL;
    *outp = l;

out:
    return ret;
}

void mcpkg_mc_loader_free(struct McPkgMcLoader *l)
{
    if (!l)
        return;

    if (l->ops && l->ops->destroy)
        l->ops->destroy(l);

    if (l->owns_base_url && l->base_url) {
        free((void *)l->base_url);
        l->base_url = NULL;
        l->owns_base_url = 0;
    }

    free(l);
}

struct McPkgMcLoader mcpkg_mc_loader_make(MCPKG_MC_LOADERS id)
{
    const struct McPkgMcLoader *hit;

    hit = mcpkg_mc_loader_find_by_id(id);
    if (hit)
        return *hit;

    return (struct McPkgMcLoader) {
        .loader = MCPKG_MC_LOADER_UNKNOWN,
        .name = mcpkg_mc_string_unknown(),
        .base_url = NULL,
        .flags = 0,
        .ops = NULL,
        .priv = NULL,
        .owns_base_url = 0,
    };
}

struct McPkgMcLoader mcpkg_mc_loader_from_string_canon(const char *s)
{
    const struct McPkgMcLoader *hit;

    hit = mcpkg_mc_loader_find_by_name(s);
    if (hit)
        return *hit;

    return mcpkg_mc_loader_make(MCPKG_MC_LOADER_UNKNOWN);
}

const char *mcpkg_mc_loader_to_string(MCPKG_MC_LOADERS id)
{
    const struct McPkgMcLoader *hit;

    hit = mcpkg_mc_loader_find_by_id(id);
    return hit ? hit->name : mcpkg_mc_string_unknown();
}

MCPKG_MC_LOADERS mcpkg_mc_loader_from_string(const char *s)
{
    const struct McPkgMcLoader *hit;

    hit = mcpkg_mc_loader_find_by_name(s);
    return hit ? hit->loader : MCPKG_MC_LOADER_UNKNOWN;
}

const struct McPkgMcLoader *mcpkg_mc_loader_table(size_t *count)
{
    if (count)
        *count = mcpkg_mc_loader_table_count();
    return g_loader_table;
}

// >0 known, 0 unknown
int mcpkg_mc_loader_is_known(MCPKG_MC_LOADERS id)
{
    return mcpkg_mc_loader_find_by_id(id) != NULL;
}

// >0 requires network, 0 does not, <0 error
int mcpkg_mc_loader_requires_network(const struct McPkgMcLoader *l)
{
    if (!l)
        return MCPKG_MC_ERR_INVALID_ARG;

    // Loaders generally do not require network; if they expose a remote API,
    // we signal that via HAS_API.
    return (l->flags & MCPKG_MC_LOADER_F_HAS_API) != 0;
}

// >0 online, 0 offline, <0 error
int mcpkg_mc_loader_is_online(struct McPkgMcLoader *l)
{
    if (!l)
        return MCPKG_MC_ERR_INVALID_ARG;

    if (l->ops && l->ops->is_online)
        return l->ops->is_online(l);

    // No cached 'online' field in loaders; default to online.
    return 1;
}

void mcpkg_mc_loader_set_base_url(struct McPkgMcLoader *l, const char *base_url)
{
    if (!l)
        return;

    if (l->owns_base_url && l->base_url) {
        free((void *)l->base_url);
        l->base_url = NULL;
        l->owns_base_url = 0;
    }

    l->base_url = base_url;
}

int mcpkg_mc_loader_set_base_url_dup(struct McPkgMcLoader *l,
                                     const char *base_url)
{
    int ret = MCPKG_MC_NO_ERROR;
    char *dup = NULL;

    if (!l)
        return MCPKG_MC_ERR_INVALID_ARG;

    if (l->owns_base_url && l->base_url) {
        free((void *)l->base_url);
        l->base_url = NULL;
        l->owns_base_url = 0;
    }

    if (base_url) {
        dup = strdup(base_url);
        if (!dup) {
            ret = MCPKG_MC_ERR_NO_MEMORY;
            goto out;
        }
        l->base_url = dup;
        l->owns_base_url = 1;
    }

out:
    return ret;
}

// Loaders don't cache 'online' in the struct; keep API for symmetry.
void mcpkg_mc_loader_set_online(struct McPkgMcLoader *l, int online)
{
    (void)l;
    (void)online;
}

void mcpkg_mc_loader_set_ops(struct McPkgMcLoader *l,
                             const struct McPkgMcLoaderOps *ops)
{
    if (!l)
        return;
    l->ops = ops;
}
