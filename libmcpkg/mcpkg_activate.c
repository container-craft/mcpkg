#include "mcpkg_activate.h"
#include "utils/mcpkg_fs.h"
#include "utils/code_names.h"
#include "mcpkg_config.h"  // to read mc_base (persisted)

MCPKG_ERROR_TYPE mcpkg_activate(const char *mc_version,
                                const char *mod_loader)
{
    if (!mc_version || !mod_loader)
        return MCPKG_ERROR_PARSE;

    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root)
        cache_root = MCPKG_CACHE;

    // read mc_base from config (or compute default)
    McPkgConfig cfg;
    if (mcpkg_cfg_load(&cfg) != 0)
        return MCPKG_ERROR_FS;

    if (!cfg.mc_base) {
#ifndef _WIN32
        const char *home = getenv("HOME");
        if (!home) {
            mcpkg_cfg_free(&cfg);
            return MCPKG_ERROR_GENERAL;
        }
        asprintf(&cfg.mc_base, "%s/.minecraft", home);
#else
        const char *appdata = getenv("APPDATA");
        if (!appdata) {
            cfg_free(&cfg);
            return MCPKG_ERROR_GENERAL;
        }
        asprintf(&cfg.mc_base, "%s/minecraft", appdata);
#endif
    }

    const char *codename = codename_for_version(mc_version);
    if (!codename) {
        mcpkg_cfg_free(&cfg);
        return MCPKG_ERROR_VERSION_MISMATCH;
    }

    // source = cache mods folder
    char src_mods[1024];
    snprintf(src_mods, sizeof src_mods,
             "%s/%s/%s/%s/mods",
             cache_root,
             mod_loader,
             codename,
             mc_version);

    // dest = MC_BASE/{mods|plugins}
    const char *leaf = target_dir_for_loader(mod_loader);
    char dst_dir[1024];
    snprintf(dst_dir, sizeof dst_dir, "%s/%s", cfg.mc_base, leaf);

    // Unix: replace with symlink to cache mods
    // if exists, remove (file/dir/link)
    mcpkg_fs_unlink(dst_dir); // best-effort
    MCPKG_ERROR_TYPE rc;
    if (mcpkg_fs_mkdir(cfg.mc_base) != MCPKG_ERROR_SUCCESS) {
        mcpkg_cfg_free(&cfg);
        return MCPKG_ERROR_FS;
    }

    // create symlink
    rc = mcpkg_fs_ln_sf(src_mods, dst_dir, 0);
    if (rc != MCPKG_ERROR_SUCCESS) {
        // fallback: copy tree if symlink fails
        if (mcpkg_fs_mkdir(dst_dir) != MCPKG_ERROR_SUCCESS) {
            mcpkg_cfg_free(&cfg);
            return MCPKG_ERROR_FS;
        }
        rc = mcpkg_fs_cp_dir(src_mods, dst_dir);
    }
    mcpkg_cfg_free(&cfg);
    return rc;
}

MCPKG_ERROR_TYPE mcpkg_cfg_deactivate(const char *mc_version, const char *mod_loader) {
    (void)mc_version; // not strictly needed for deactivate
    if (!mod_loader)
        return MCPKG_ERROR_PARSE;

    McPkgConfig cfg;
    if (mcpkg_cfg_load(&cfg) != 0)
        return MCPKG_ERROR_FS;

    if (!cfg.mc_base) {
        mcpkg_cfg_free(&cfg);
        return MCPKG_ERROR_GENERAL;
    }

    const char *leaf = target_dir_for_loader(mod_loader);
    char dst_dir[1024];
    snprintf(dst_dir, sizeof dst_dir, "%s/%s", cfg.mc_base, leaf);

#ifdef _WIN32
    mcpkg_fs_rm_dir(dst_dir);
#else
    mcpkg_fs_unlink(dst_dir);
#endif

    mcpkg_cfg_free(&cfg);
    return MCPKG_ERROR_SUCCESS;
}
