#include <stdlib.h>
#include <string.h>
#include "mc/mcpkg_mc_versions.h"
#include "mc/mcpkg_mc_util.h"
static const char *const g_codename_slugs[] = {
    "unknown",
    "tricky_trials",
    "trails_and_tales",
    "the_wild",
    "caves_and_cliffs_two",
    "caves_and_cliffs_one",
    "nether_update",
    "buzzy_bees",
    "village_and_pillage",
    "aquatic",
    "world_of_color",
    "exploration",
    "frostburn",
    "combat",
    "bountiful",
    "changed_the_world",
    "horse",
    "redstone",
    "pretty_scary",
    "villager_trading",
    "faithful",
    "spawn_egg",
    "adventure",
};

static size_t mcpkg__codename_count(void)
{
    return sizeof(g_codename_slugs) / sizeof(g_codename_slugs[0]);
}

const char *mcpkg_mc_codename_to_string(MCPKG_MC_CODE_NAME code)
{
    size_t n = mcpkg__codename_count();
    return (code >= 0 && (size_t)code < n) ? g_codename_slugs[code]
                                            : g_codename_slugs[0];
}

MCPKG_MC_CODE_NAME mcpkg_mc_codename_from_string(const char *s)
{
    size_t i, n;

    if (!s)
        return MCPKG_MC_CODE_NAME_UNKNOWN;

    n = mcpkg__codename_count();
    for (i = 0; i < n; i++) {
        if (mcpkg_mc_ascii_ieq(g_codename_slugs[i], s))
            return (MCPKG_MC_CODE_NAME)i;
    }
    return MCPKG_MC_CODE_NAME_UNKNOWN;
}

struct McPkgMCVersion mcpkg_mc_version_family_make(MCPKG_MC_CODE_NAME code)
{
    return (struct McPkgMCVersion) {
        .codename = code,
        .snapshot = 0,
        .versions = NULL,
    };
}

int mcpkg_mc_version_family_new(struct McPkgMCVersion **out,
                                MCPKG_MC_CODE_NAME code)
{
    struct McPkgMCVersion *v;

    if (!out)
        return MCPKG_MC_ERR_INVALID_ARG;

    v = (struct McPkgMCVersion *)calloc(1, sizeof(*v));
    if (!v)
        return MCPKG_MC_ERR_NO_MEMORY;

    v->codename = code;
    v->snapshot = 0;
    v->versions = mcpkg_stringlist_new(0, 0);
    if (!v->versions) {
        free(v);
        return MCPKG_MC_ERR_NO_MEMORY;
    }

    *out = v;
    return MCPKG_MC_NO_ERROR;
}

void mcpkg_mc_version_family_free(struct McPkgMCVersion *v)
{
    if (!v)
        return;

    if (v->versions)
        mcpkg_stringlist_free(v->versions);

    free(v);
}

int mcpkg_mc_version_family_latest(const struct McPkgMCVersion *v,
                                   const char **out_latest)
{
    if (!v || !out_latest)
        return MCPKG_MC_ERR_INVALID_ARG;
    if (!v->versions || mcpkg_stringlist_size(v->versions) == 0) {
        *out_latest = NULL;
        return 0;
    }
    *out_latest = mcpkg_stringlist_at(v->versions, 0);
    return (*out_latest != NULL) ? 1 : 0;
}

MCPKG_MC_CODE_NAME
mcpkg_mc_codename_from_version(const struct McPkgMCVersion *const *families,
                               size_t nfam, const char *mc_version)
{
    size_t i;

    if (!families || !mc_version)
        return MCPKG_MC_CODE_NAME_UNKNOWN;

    for (i = 0; i < nfam; i++) {
        const struct McPkgMCVersion *vf = families[i];
        size_t j, n;

        if (!vf || !vf->versions)
            continue;

        n = mcpkg_stringlist_size(vf->versions);
        for (j = 0; j < n; j++) {
            const char *v = mcpkg_stringlist_at(vf->versions, j);
            if (v && strcmp(v, mc_version) == 0)
                return vf->codename;
        }
    }
    return MCPKG_MC_CODE_NAME_UNKNOWN;
}
