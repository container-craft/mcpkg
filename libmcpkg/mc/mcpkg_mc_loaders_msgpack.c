#include <stdlib.h>
#include <string.h>

#include "mc/mcpkg_mc_loaders.h"
#include "mp/mcpkg_mp_util.h"
#include "mc/mcpkg_mc_util.h"


// int-keyed fields (0/1 reserved for common TAG/VER)
#define MC_LDR_K_LOADER     2
#define MC_LDR_K_NAME       3
#define MC_LDR_K_BASE_URL   4
#define MC_LDR_K_FLAGS      5

int mcpkg_mc_loader_pack(const struct McPkgMcLoader *l,
                         void **out_buf, size_t *out_len)
{
    int ret = MCPKG_MC_NO_ERROR;
    struct McPkgMpWriter w;
    int mpret;

    if (!l || !out_buf || !out_len)
        return MCPKG_MC_ERR_INVALID_ARG;

    mpret = mcpkg_mp_writer_init(&w);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out;
    }

    // 2 header keys (TAG, VER) + 4 fields
    mpret = mcpkg_mp_map_begin(&w, 6);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_w;
    }

    mpret = mcpkg_mp_write_header(&w, MCPKG_MC_LOADER_MP_TAG,
                                  MCPKG_MC_LOADER_MP_VERSION);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_w;
    }

    // loader id (required)
    mpret = mcpkg_mp_kv_i32(&w, MC_LDR_K_LOADER, (int)l->loader);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_w;
    }

    // canonical name (optional hint for inspectors)
    mpret = mcpkg_mp_kv_str(&w, MC_LDR_K_NAME, l->name);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_w;
    }

    // base_url (nil if none)
    if (l->base_url)
        mpret = mcpkg_mp_kv_str(&w, MC_LDR_K_BASE_URL, l->base_url);
    else
        mpret = mcpkg_mp_kv_nil(&w, MC_LDR_K_BASE_URL);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_w;
    }

    // flags
    mpret = mcpkg_mp_kv_u32(&w, MC_LDR_K_FLAGS, l->flags);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_w;
    }

    // finalize buffer
    mpret = mcpkg_mp_writer_finish(&w, out_buf, out_len);
    if (mpret != MCPKG_MP_NO_ERROR)
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);

out_w:
    mcpkg_mp_writer_destroy(&w);
out:
    return ret;
}

int mcpkg_mc_loader_unpack(const void *buf, size_t len,
                           struct McPkgMcLoader *out)
{
    int ret = MCPKG_MC_NO_ERROR;
    struct McPkgMpReader r;
    int mpret, found, ver = 0;
    int64_t i64 = 0;
    uint32_t u32 = 0;

    MCPKG_MC_LOADERS loader = MCPKG_MC_LOADER_UNKNOWN;
    uint32_t flags = 0;
    const char *base_url_ptr = NULL;
    size_t base_url_len = 0;

    if (!buf || !len || !out)
        return MCPKG_MC_ERR_INVALID_ARG;

    mpret = mcpkg_mp_reader_init(&r, buf, len);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out;
    }

    mpret = mcpkg_mp_expect_tag(&r, MCPKG_MC_LOADER_MP_TAG, &ver);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_r; }
    // ver kept for future evolution (currently v1)

    // loader (required)
    mpret = mcpkg_mp_get_i64(&r, MC_LDR_K_LOADER, &i64, &found);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_r;
    }
    if (!found) {
        ret = MCPKG_MC_ERR_PARSE;
        goto out_r;
    }
    loader = (MCPKG_MC_LOADERS)i64;

    // base_url (optional)
    mpret = mcpkg_mp_get_str_borrow(&r, MC_LDR_K_BASE_URL,
                                    &base_url_ptr, &base_url_len, &found);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_r;
    }

    // flags (optional -> default 0)
    mpret = mcpkg_mp_get_u32(&r, MC_LDR_K_FLAGS, &u32, &found);
    if (mpret != MCPKG_MP_NO_ERROR) {
        ret = mcpkg_mc_err_from_mcpkg_pack_err(mpret);
        goto out_r;
    }
    if (found)
        flags = u32;

    // Initialize from template for canonical name and defaults
    *out = mcpkg_mc_loader_make(loader);

    // base_url: duplicate if present; manage ownership
    if (out->owns_base_url && out->base_url) {
        free((void *)out->base_url);
        out->base_url = NULL;
        out->owns_base_url = 0;
    }
    if (base_url_ptr && base_url_len) {
        char *dup;

        dup = (char *)malloc(base_url_len + 1);
        if (!dup) {
            ret = MCPKG_MC_ERR_NO_MEMORY;
            goto out_r;
        }

        memcpy(dup, base_url_ptr, base_url_len);
        dup[base_url_len] = '\0';
        out->base_url = dup;
        out->owns_base_url = 1;
    }

    out->flags = flags;

    ret = MCPKG_MC_NO_ERROR;

out_r:
    mcpkg_mp_reader_destroy(&r);
out:
    return ret;
}
