#include "activate.h"
#include "utils/mcpkg_fs.h"
#include "utils/code_names.h"
#include "mcpkg_config.h"  // to read mc_base (persisted)
mcpkg_error_types mcpkg_activate(const char *mc_version, const char *mod_loader) {
    if (!mc_version || !mod_loader) return MCPKG_ERROR_PARSE;

    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root)
        cache_root = MCPKG_CACHE;

    // read mc_base from config (or compute default)
    McpkgConfig cfg;
    if (cfg_load(&cfg) != 0)
        return MCPKG_ERROR_FS;
    if (!cfg.mc_base) {
#ifndef _WIN32
        const char *home = getenv("HOME");
        if (!home) {
            cfg_free(&cfg);
            return MCPKG_ERROR_GENERAL;
        }
        asprintf(&cfg.mc_base, "%s/.minecraft", home);
#else
        const char *appdata = getenv("APPDATA");
        if (!appdata) {
            cfg_free(&cfg);
            return MCPKG_ERROR_GENERAL;
        }
        asprintf(&cfg.mc_base, "%s/.minecraft", appdata);
#endif
    }

    const char *codename = codename_for_version(mc_version);
    if (!codename) {
        cfg_free(&cfg);
        return MCPKG_ERROR_VERSION_MISMATCH;
    }

    // source = cache mods folder
    char src_mods[1024];
    snprintf(src_mods, sizeof src_mods, "%s/%s/%s/%s/mods", cache_root, mod_loader, codename, mc_version);

    // dest = MC_BASE/{mods|plugins}
    const char *leaf = target_dir_for_loader(mod_loader);
    char dst_dir[1024];
    snprintf(dst_dir, sizeof dst_dir, "%s/%s", cfg.mc_base, leaf);

#ifdef _WIN32
    // Windows: recursive copy
    if (mkdir_p(dst_dir) != MCPKG_ERROR_SUCCESS) { cfg_free(&cfg); return MCPKG_ERROR_FS; }
    mcpkg_error_types rc = mcpkg_copy_tree(src_mods, dst_dir);
    cfg_free(&cfg);
    return rc;
#else
    // Unix: replace with symlink to cache mods
    // if exists, remove (file/dir/link)
    mcpkg_unlink_any(dst_dir); // best-effort
    mcpkg_error_types rc;
    if (mkdir_p(cfg.mc_base) != MCPKG_ERROR_SUCCESS) {
        cfg_free(&cfg);
        return MCPKG_ERROR_FS;
    }

    // create symlink
    rc = create_symlink(src_mods, dst_dir);
    if (rc != MCPKG_ERROR_SUCCESS) {
        // fallback: copy tree if symlink fails
        if (mkdir_p(dst_dir) != MCPKG_ERROR_SUCCESS) {
            cfg_free(&cfg);
            return MCPKG_ERROR_FS;
        }
        rc = mcpkg_copy_tree(src_mods, dst_dir);
    }
    cfg_free(&cfg);
    return rc;
#endif
}

mcpkg_error_types mcpkg_deactivate(const char *mc_version, const char *mod_loader) {
    (void)mc_version; // not strictly needed for deactivate
    if (!mod_loader)
        return MCPKG_ERROR_PARSE;

    // read mc_base
    McpkgConfig cfg;
    if (cfg_load(&cfg) != 0)
        return MCPKG_ERROR_FS;
    if (!cfg.mc_base) {
        cfg_free(&cfg);
        return MCPKG_ERROR_GENERAL;
    }

    const char *leaf = target_dir_for_loader(mod_loader);
    char dst_dir[1024];
    snprintf(dst_dir, sizeof dst_dir, "%s/%s", cfg.mc_base, leaf);

#ifdef _WIN32
    // Windows: remove the copied tree (I have no Idea whats going on man screw doz)
    mcpkg_error_types rc = mcpkg_remove_tree(dst_dir);
    // optionally recreate empty dir:
    mkdir_p(dst_dir);
    cfg_free(&cfg);
    return rc;
#else
    mcpkg_unlink_any(dst_dir);
    mkdir_p(dst_dir);
    cfg_free(&cfg);
    return MCPKG_ERROR_SUCCESS;
#endif
}
