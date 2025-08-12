#include <stdlib.h>
#include <string.h>

#include "mc/mcpkg_mc.h"

static int mc_list_push_ptr(McPkgList **lstp, void *ptr)
{
    if (!*lstp) {
        // elem_size = sizeof(void*), no custom ops, default limits
        *lstp = mcpkg_list_new(sizeof(void *), NULL, 0, 0);
        if (!*lstp)
            return MCPKG_MC_ERR_NO_MEMORY;
    }

    return (mcpkg_list_push(*lstp, &ptr) == MCPKG_CONTAINER_OK)
               ? MCPKG_MC_NO_ERROR
               : MCPKG_MC_ERR_NO_MEMORY;
}

static size_t mc_list_size(const McPkgList *lst)
{
    return lst ? mcpkg_list_size(lst) : 0;
}

static void **mc_list_cell_ptr(McPkgList *lst, size_t idx)
{
    return (void **)mcpkg_list_at_ptr(lst, idx);
}

static void mc_free_provider(struct McPkgMcProvider *p)
{
    mcpkg_mc_provider_free(p);
}

static void mc_free_loader(struct McPkgMcLoader *l)
{
    mcpkg_mc_loader_free(l);
}

static void mc_free_version_family(struct McPkgMCVersion *vf)
{
    mcpkg_mc_version_family_free(vf);
}

static void mc_clear_list_of_ptrs(McPkgList *lst, void (*free_fn)(void *))
{
    size_t i, n;

    if (!lst)
        return;

    n = mcpkg_list_size(lst);
    for (i = 0; i < n; i++) {
        void **cell = mc_list_cell_ptr(lst, i);
        if (cell && *cell) {
            free_fn(*cell);
            *cell = NULL;
        }
    }
    mcpkg_list_free(lst);
}

// ---- Lifecycle --------------------------------------------------------------

int mcpkg_mc_new(struct McPkgMc **out)
{
    struct McPkgMc *mc;

    if (!out)
        return MCPKG_MC_ERR_INVALID_ARG;

    mc = (struct McPkgMc *)calloc(1, sizeof(*mc));
    if (!mc)
        return MCPKG_MC_ERR_NO_MEMORY;

    *out = mc;
    return MCPKG_MC_NO_ERROR;
}

void mcpkg_mc_free(struct McPkgMc *mc)
{
    if (!mc)
        return;

    if (mc->current_provider) {
        mcpkg_mc_provider_free(mc->current_provider);
        mc->current_provider = NULL;
    }
    if (mc->current_loader) {
        mcpkg_mc_loader_free(mc->current_loader);
        mc->current_loader = NULL;
    }
    if (mc->current_version) {
        mcpkg_mc_version_family_free(mc->current_version);
        mc->current_version = NULL;
    }

    // Registries (lists of owned pointers)
    mc_clear_list_of_ptrs(mc->providers, (void (*)(void *))mc_free_provider);
    mc_clear_list_of_ptrs(mc->loaders, (void (*)(void *))mc_free_loader);
    mc_clear_list_of_ptrs(mc->versions, (void (*)(void *))mc_free_version_family);

    free(mc);
}

// ---- Optional singleton -----------------------------------------------------

static struct McPkgMc *g_mc_singleton;

int mcpkg_mc_global_init(void)
{
    if (g_mc_singleton)
        return MCPKG_MC_NO_ERROR;

    return mcpkg_mc_new(&g_mc_singleton);
}

struct McPkgMc *mcpkg_mc_global(void)
{
    return g_mc_singleton;
}

void mcpkg_mc_global_shutdown(void)
{
    if (!g_mc_singleton)
        return;
    mcpkg_mc_free(g_mc_singleton);
    g_mc_singleton = NULL;
}

// ---- Seeding ---------------------------------------------------------------

int mcpkg_mc_seed_providers(struct McPkgMc *mc)
{
    size_t i, n;
    const struct McPkgMcProvider *tbl;

    if (!mc)
        return MCPKG_MC_ERR_INVALID_ARG;

    tbl = mcpkg_mc_provider_table(&n);
    for (i = 0; i < n; i++) {
        struct McPkgMcProvider *p = NULL;
        int rc;

        rc = mcpkg_mc_provider_new(&p, tbl[i].provider);
        if (rc != MCPKG_MC_NO_ERROR)
            return rc;

        rc = mc_list_push_ptr(&mc->providers, p);
        if (rc != MCPKG_MC_NO_ERROR) {
            mcpkg_mc_provider_free(p);
            return rc;
        }
    }
    return MCPKG_MC_NO_ERROR;
}

int mcpkg_mc_seed_loaders(struct McPkgMc *mc)
{
    size_t i, n;
    const struct McPkgMcLoader *tbl;

    if (!mc)
        return MCPKG_MC_ERR_INVALID_ARG;

    tbl = mcpkg_mc_loader_table(&n);
    for (i = 0; i < n; i++) {
        struct McPkgMcLoader *l = NULL;
        int rc;

        rc = mcpkg_mc_loader_new(&l, tbl[i].loader);
        if (rc != MCPKG_MC_NO_ERROR)
            return rc;

        rc = mc_list_push_ptr(&mc->loaders, l);
        if (rc != MCPKG_MC_NO_ERROR) {
            mcpkg_mc_loader_free(l);
            return rc;
        }
    }
    return MCPKG_MC_NO_ERROR;
}

// Minimal seed: one family with the default version so code paths work.
int mcpkg_mc_seed_versions_minimal(struct McPkgMc *mc)
{
    struct McPkgMCVersion *vf = NULL;
    int rc;

    if (!mc)
        return MCPKG_MC_ERR_INVALID_ARG;

    rc = mcpkg_mc_version_family_new(&vf, MCPKG_MC_CODE_NAME_TRICKY_TRIALS);
    if (rc != MCPKG_MC_NO_ERROR)
        return rc;

    // Add the default version as latest (env default).
    if (vf->versions) {
        (void)mcpkg_stringlist_push(vf->versions, MCPKG_MC_DEFAULT_VERSION);
    }

    rc = mc_list_push_ptr(&mc->versions, vf);
    if (rc != MCPKG_MC_NO_ERROR) {
        mcpkg_mc_version_family_free(vf);
        return rc;
    }
    return MCPKG_MC_NO_ERROR;
}

// Placeholder full seed — call minimal for now; replace with canon set later.
int mcpkg_mc_seed_versions_all(struct McPkgMc *mc)
{
    return mcpkg_mc_seed_versions_minimal(mc);
}

// ---- Add / Own -------------------------------------------------------------

int mcpkg_mc_add_provider(struct McPkgMc *mc, struct McPkgMcProvider *p)
{
    if (!mc || !p)
        return MCPKG_MC_ERR_INVALID_ARG;

    return mc_list_push_ptr(&mc->providers, p);
}

int mcpkg_mc_add_loader(struct McPkgMc *mc, struct McPkgMcLoader *l)
{
    if (!mc || !l)
        return MCPKG_MC_ERR_INVALID_ARG;

    return mc_list_push_ptr(&mc->loaders, l);
}

int mcpkg_mc_add_version_family(struct McPkgMc *mc, struct McPkgMCVersion *vf)
{
    if (!mc || !vf)
        return MCPKG_MC_ERR_INVALID_ARG;

    return mc_list_push_ptr(&mc->versions, vf);
}

// ---- Find helpers (borrowed) -----------------------------------------------

struct McPkgMcProvider *mcpkg_mc_find_provider_id(struct McPkgMc *mc,
                                                  MCPKG_MC_PROVIDERS id)
{
    size_t i, n;

    if (!mc || !mc->providers)
        return NULL;

    n = mc_list_size(mc->providers);
    for (i = 0; i < n; i++) {
        struct McPkgMcProvider **cell =
            (struct McPkgMcProvider **)mc_list_cell_ptr(mc->providers, i);
        if (cell && *cell && (*cell)->provider == id)
            return *cell;
    }
    return NULL;
}

struct McPkgMcProvider *mcpkg_mc_find_provider_name(struct McPkgMc *mc,
                                                    const char *name)
{
    size_t i, n;

    if (!mc || !mc->providers || !name)
        return NULL;

    n = mc_list_size(mc->providers);
    for (i = 0; i < n; i++) {
        struct McPkgMcProvider **cell =
            (struct McPkgMcProvider **)mc_list_cell_ptr(mc->providers, i);
        if (cell && *cell && mcpkg_mc_ascii_ieq((*cell)->name, name))
            return *cell;
    }
    return NULL;
}

struct McPkgMcLoader *mcpkg_mc_find_loader_id(struct McPkgMc *mc,
                                              MCPKG_MC_LOADERS id)
{
    size_t i, n;

    if (!mc || !mc->loaders)
        return NULL;

    n = mc_list_size(mc->loaders);
    for (i = 0; i < n; i++) {
        struct McPkgMcLoader **cell =
            (struct McPkgMcLoader **)mc_list_cell_ptr(mc->loaders, i);
        if (cell && *cell && (*cell)->loader == id)
            return *cell;
    }
    return NULL;
}

struct McPkgMcLoader *mcpkg_mc_find_loader_name(struct McPkgMc *mc,
                                                const char *name)
{
    size_t i, n;

    if (!mc || !mc->loaders || !name)
        return NULL;

    n = mc_list_size(mc->loaders);
    for (i = 0; i < n; i++) {
        struct McPkgMcLoader **cell =
            (struct McPkgMcLoader **)mc_list_cell_ptr(mc->loaders, i);
        if (cell && *cell && mcpkg_mc_ascii_ieq((*cell)->name, name))
            return *cell;
    }
    return NULL;
}

struct McPkgMCVersion *mcpkg_mc_find_family_code(struct McPkgMc *mc,
                                                 MCPKG_MC_CODE_NAME code)
{
    size_t i, n;

    if (!mc || !mc->versions)
        return NULL;

    n = mc_list_size(mc->versions);
    for (i = 0; i < n; i++) {
        struct McPkgMCVersion **cell =
            (struct McPkgMCVersion **)mc_list_cell_ptr(mc->versions, i);
        if (cell && *cell && (*cell)->codename == code)
            return *cell;
    }
    return NULL;
}

struct McPkgMCVersion *mcpkg_mc_find_family_slug(struct McPkgMc *mc,
                                                 const char *codename_slug)
{
    MCPKG_MC_CODE_NAME code;

    if (!codename_slug)
        return NULL;

    code = mcpkg_mc_codename_from_string(codename_slug);
    return mcpkg_mc_find_family_code(mc, code);
}

// ---- Current selections -----------------------------------------------------

static void mc_swap_current_provider(struct McPkgMc *mc,
                                     struct McPkgMcProvider *p)
{
    if (mc->current_provider)
        mcpkg_mc_provider_free(mc->current_provider);
    mc->current_provider = p;
}

static void mc_swap_current_loader(struct McPkgMc *mc,
                                   struct McPkgMcLoader *l)
{
    if (mc->current_loader)
        mcpkg_mc_loader_free(mc->current_loader);
    mc->current_loader = l;
}

static void mc_swap_current_family(struct McPkgMc *mc,
                                   struct McPkgMCVersion *vf)
{
    if (mc->current_version)
        mcpkg_mc_version_family_free(mc->current_version);
    mc->current_version = vf;
}

int mcpkg_mc_set_current_provider(struct McPkgMc *mc,
                                  struct McPkgMcProvider *p)
{
    if (!mc || !p)
        return MCPKG_MC_ERR_INVALID_ARG;
    mc_swap_current_provider(mc, p);
    return MCPKG_MC_NO_ERROR;
}

int mcpkg_mc_set_current_provider_id(struct McPkgMc *mc, MCPKG_MC_PROVIDERS id)
{
    struct McPkgMcProvider *p;

    if (!mc)
        return MCPKG_MC_ERR_INVALID_ARG;

    p = mcpkg_mc_find_provider_id(mc, id);
    if (!p)
        return MCPKG_MC_ERR_NOT_FOUND;

    // Keep a private owned copy as "current"
    {
        struct McPkgMcProvider *copy = NULL;
        int rc = mcpkg_mc_provider_new(&copy, p->provider);
        if (rc != MCPKG_MC_NO_ERROR)
            return rc;
        // Mirror base_url/flags if they were customized in registry.
        if (p->base_url)
            (void)mcpkg_mc_provider_set_base_url_dup(copy, p->base_url);
        copy->flags = p->flags;
        mc_swap_current_provider(mc, copy);
    }
    return MCPKG_MC_NO_ERROR;
}

int mcpkg_mc_set_current_loader(struct McPkgMc *mc, struct McPkgMcLoader *l)
{
    if (!mc || !l)
        return MCPKG_MC_ERR_INVALID_ARG;
    mc_swap_current_loader(mc, l);
    return MCPKG_MC_NO_ERROR;
}

int mcpkg_mc_set_current_loader_id(struct McPkgMc *mc, MCPKG_MC_LOADERS id)
{
    struct McPkgMcLoader *l;

    if (!mc)
        return MCPKG_MC_ERR_INVALID_ARG;

    l = mcpkg_mc_find_loader_id(mc, id);
    if (!l)
        return MCPKG_MC_ERR_NOT_FOUND;

    {
        struct McPkgMcLoader *copy = NULL;
        int rc = mcpkg_mc_loader_new(&copy, l->loader);
        if (rc != MCPKG_MC_NO_ERROR)
            return rc;
        if (l->base_url)
            (void)mcpkg_mc_loader_set_base_url_dup(copy, l->base_url);
        copy->flags = l->flags;
        mc_swap_current_loader(mc, copy);
    }
    return MCPKG_MC_NO_ERROR;
}

int mcpkg_mc_set_current_family(struct McPkgMc *mc, struct McPkgMCVersion *vf)
{
    if (!mc || !vf)
        return MCPKG_MC_ERR_INVALID_ARG;
    mc_swap_current_family(mc, vf);
    return MCPKG_MC_NO_ERROR;
}

int mcpkg_mc_set_current_family_code(struct McPkgMc *mc,
                                     MCPKG_MC_CODE_NAME code)
{
    struct McPkgMCVersion *vf;

    if (!mc)
        return MCPKG_MC_ERR_INVALID_ARG;

    vf = mcpkg_mc_find_family_code(mc, code);
    if (!vf)
        return MCPKG_MC_ERR_NOT_FOUND;

    // Make a deep copy of the family (own current)
    {
        struct McPkgMCVersion *copy = NULL;
        int rc = mcpkg_mc_version_family_new(&copy, vf->codename);
        if (rc != MCPKG_MC_NO_ERROR)
            return rc;

        copy->snapshot = vf->snapshot;
        if (vf->versions) {
            size_t i, n = mcpkg_stringlist_size(vf->versions);
            for (i = 0; i < n; i++)
                (void)mcpkg_stringlist_push(copy->versions,
                                             mcpkg_stringlist_at(vf->versions, i));
        }
        mc_swap_current_family(mc, copy);
    }
    return MCPKG_MC_NO_ERROR;
}

// ---- Convenience lookups ----------------------------------------------------

int mcpkg_mc_latest_for_codename(struct McPkgMc *mc,
                                 MCPKG_MC_CODE_NAME code,
                                 const char **out_version)
{
    struct McPkgMCVersion *vf;

    if (!mc || !out_version)
        return MCPKG_MC_ERR_INVALID_ARG;

    vf = mcpkg_mc_find_family_code(mc, code);
    if (!vf) {
        *out_version = NULL;
        return 0;
    }
    return mcpkg_mc_version_family_latest(vf, out_version);
}

MCPKG_MC_CODE_NAME
mcpkg_mc_codename_from_version_in(struct McPkgMc *mc, const char *mc_version)
{
    size_t i, n;
    const struct McPkgMCVersion **arr = NULL;
    MCPKG_MC_CODE_NAME code = MCPKG_MC_CODE_NAME_UNKNOWN;

    if (!mc || !mc->versions || !mc_version)
        return MCPKG_MC_CODE_NAME_UNKNOWN;

    n = mc_list_size(mc->versions);
    if (!n)
        return MCPKG_MC_CODE_NAME_UNKNOWN;

    arr = (const struct McPkgMCVersion **)
        calloc(n, sizeof(*arr));
    if (!arr)
        return MCPKG_MC_CODE_NAME_UNKNOWN;

    for (i = 0; i < n; i++) {
        struct McPkgMCVersion **cell =
            (struct McPkgMCVersion **)mc_list_cell_ptr(mc->versions, i);
        arr[i] = cell ? *cell : NULL;
    }

    code = mcpkg_mc_codename_from_version(arr, n, mc_version);
    free(arr);
    return code;
}

// ---- Current selection (de)serialization -----------------------------------

int mcpkg_mc_pack_current_provider(const struct McPkgMc *mc,
                                   void **out_buf, size_t *out_len)
{
    if (!mc || !mc->current_provider)
        return MCPKG_MC_ERR_INVALID_ARG;

    return mcpkg_mc_provider_pack(mc->current_provider, out_buf, out_len);
}

int mcpkg_mc_unpack_current_provider(struct McPkgMc *mc,
                                     const void *buf, size_t len)
{
    struct McPkgMcProvider *p;

    if (!mc || !buf || !len)
        return MCPKG_MC_ERR_INVALID_ARG;

    // Allocate raw storage and unpack into it.
    p = (struct McPkgMcProvider *)calloc(1, sizeof(*p));
    if (!p)
        return MCPKG_MC_ERR_NO_MEMORY;

    {
        int rc = mcpkg_mc_provider_unpack(buf, len, p);
        if (rc != MCPKG_MC_NO_ERROR) {
            free(p);
            return rc;
        }
    }
    mc_swap_current_provider(mc, p);
    return MCPKG_MC_NO_ERROR;
}

int mcpkg_mc_pack_current_loader(const struct McPkgMc *mc,
                                 void **out_buf, size_t *out_len)
{
    if (!mc || !mc->current_loader)
        return MCPKG_MC_ERR_INVALID_ARG;

    return mcpkg_mc_loader_pack(mc->current_loader, out_buf, out_len);
}

int mcpkg_mc_unpack_current_loader(struct McPkgMc *mc,
                                   const void *buf, size_t len)
{
    struct McPkgMcLoader *l;

    if (!mc || !buf || !len)
        return MCPKG_MC_ERR_INVALID_ARG;

    l = (struct McPkgMcLoader *)calloc(1, sizeof(*l));
    if (!l)
        return MCPKG_MC_ERR_NO_MEMORY;

    {
        int rc = mcpkg_mc_loader_unpack(buf, len, l);
        if (rc != MCPKG_MC_NO_ERROR) {
            free(l);
            return rc;
        }
    }
    mc_swap_current_loader(mc, l);
    return MCPKG_MC_NO_ERROR;
}

int mcpkg_mc_pack_current_family(const struct McPkgMc *mc,
                                 void **out_buf, size_t *out_len)
{
    if (!mc || !mc->current_version)
        return MCPKG_MC_ERR_INVALID_ARG;

    return mcpkg_mc_version_family_pack(mc->current_version, out_buf, out_len);
}

int mcpkg_mc_unpack_current_family(struct McPkgMc *mc,
                                   const void *buf, size_t len)
{
    struct McPkgMCVersion *vf;

    if (!mc || !buf || !len)
        return MCPKG_MC_ERR_INVALID_ARG;

    vf = (struct McPkgMCVersion *)calloc(1, sizeof(*vf));
    if (!vf)
        return MCPKG_MC_ERR_NO_MEMORY;

    {
        int rc = mcpkg_mc_version_family_unpack(buf, len, vf);
        if (rc != MCPKG_MC_NO_ERROR) {
            free(vf);
            return rc;
        }
    }
    mc_swap_current_family(mc, vf);
    return MCPKG_MC_NO_ERROR;
}
