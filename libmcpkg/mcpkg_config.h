#ifndef MCPKG_CONFIG_H
#define MCPKG_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mcpkg.h"
#include "utils/mcpkg_fs.h"   // mkdir_p

typedef struct {
    char *mc_base;
    char *mc_version;
    char *mc_loader;
} McpkgConfig;

/* --- helpers ------------------------------------------------------------ */

static void cfg_free(McpkgConfig *cfg) {
    if (!cfg) return;
    free(cfg->mc_base);
    free(cfg->mc_version);
    free(cfg->mc_loader);
    memset(cfg, 0, sizeof(*cfg));
}

/* Build config dir path using forward slashes so mkdir_p() works on all OSes. */
static char *get_config_dir(void) {
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    if (!appdata) return NULL;
    char *out = NULL;
    if (asprintf(&out, "%s/mcpkg", appdata) < 0) return NULL;   // use '/'
    return out;
#else
    const char *xdg = getenv("XDG_CONFIG_HOME");
    char *out = NULL;
    if (xdg && *xdg) {
        if (asprintf(&out, "%s/mcpkg", xdg) < 0) return NULL;
        return out;
    }
    const char *home = getenv("HOME");
    if (!home) return NULL;
    if (asprintf(&out, "%s/.config/mcpkg", home) < 0) return NULL;
    return out;
#endif
}

static char *get_config_file(void) {
    char *dir = get_config_dir();
    if (!dir) return NULL;
    char *out = NULL;
    if (asprintf(&out, "%s/config", dir) < 0) { free(dir); return NULL; }
    free(dir);
    return out;
}

/* Load key=value config; missing file => success with empty cfg. */
static int cfg_load(McpkgConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    char *path = get_config_file();
    if (!path) return -1;

    FILE *fp = fopen(path, "r");
    if (!fp) { free(path); return 0; } // no config yet is fine

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        size_t n = strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';
        if (!n || line[0] == '#') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *key = line;
        const char *val = eq + 1;

        if (strcmp(key, "mc_base") == 0)       cfg->mc_base    = strdup(val);
        else if (strcmp(key, "mc_version") == 0) cfg->mc_version = strdup(val);
        else if (strcmp(key, "mc_loader") == 0)  cfg->mc_loader  = strdup(val);
    }
    fclose(fp);
    free(path);
    return 0;
}

static int cfg_save(const McpkgConfig *cfg) {
    char *dir = get_config_dir();
    if (!dir) return -1;
    if (mkdir_p(dir) != MCPKG_ERROR_SUCCESS) { free(dir); return -1; }
    free(dir);

    char *path = get_config_file();
    if (!path) return -1;

    FILE *fp = fopen(path, "w");
    if (!fp) { free(path); return -1; }

    if (cfg->mc_base)
        fprintf(fp, "mc_base=%s\n",    cfg->mc_base);
    if (cfg->mc_version)
        fprintf(fp, "mc_version=%s\n", cfg->mc_version);
    if (cfg->mc_loader)
        fprintf(fp, "mc_loader=%s\n",  cfg->mc_loader);

    int ok = (fclose(fp) == 0) ? 0 : -1;
    free(path);
    return ok;
}

static inline mcpkg_error_types mcpkg_global_mc_base(const char *mc_base)
{
    if (!mc_base || !*mc_base) return MCPKG_ERROR_PARSE;

    McpkgConfig cfg;
    if (cfg_load(&cfg) != 0) return MCPKG_ERROR_FS;

    free(cfg.mc_base);
    cfg.mc_base = strdup(mc_base);
    if (!cfg.mc_base && *mc_base) {
        cfg_free(&cfg);
        return MCPKG_ERROR_OOM;
    }

    int rc = cfg_save(&cfg);
    cfg_free(&cfg);
    return (rc == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

static inline mcpkg_error_types mcpkg_global_mc_version(const char *mc_version)
{
    if (!mc_version || !*mc_version) return MCPKG_ERROR_PARSE;

    McpkgConfig cfg;
    if (cfg_load(&cfg) != 0) return MCPKG_ERROR_FS;

    free(cfg.mc_version);
    cfg.mc_version = strdup(mc_version);
    if (!cfg.mc_version && *mc_version) { cfg_free(&cfg); return MCPKG_ERROR_OOM; }

    int rc = cfg_save(&cfg);
    cfg_free(&cfg);
    return (rc == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

static inline mcpkg_error_types mcpkg_global_mc_loader(const char *mc_loader)
{
    if (!mc_loader || !*mc_loader) return MCPKG_ERROR_PARSE;

    McpkgConfig cfg;
    if (cfg_load(&cfg) != 0) return MCPKG_ERROR_FS;

    free(cfg.mc_loader);
    cfg.mc_loader = strdup(mc_loader);
    if (!cfg.mc_loader && *mc_loader) { cfg_free(&cfg); return MCPKG_ERROR_OOM; }

    int rc = cfg_save(&cfg);
    cfg_free(&cfg);
    return (rc == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

/**
 * Initialize config defaults if missing and persist them.
 * (Env vars still override at runtime in your CLI.)
 */
static inline mcpkg_error_types mcpkg_init(void)
{
    McpkgConfig cfg;
    if (cfg_load(&cfg) != 0) return MCPKG_ERROR_FS;

    if (!cfg.mc_base) {
#ifdef _WIN32
        const char *appdata = getenv("APPDATA");
        if (appdata) {
            if (asprintf(&cfg.mc_base, "%s/.minecraft", appdata) < 0) { cfg_free(&cfg); return MCPKG_ERROR_OOM; }
        }
#else
        const char *home = getenv("HOME");
        if (home) {
            if (asprintf(&cfg.mc_base, "%s/.minecraft", home) < 0) { cfg_free(&cfg); return MCPKG_ERROR_OOM; }
        }
#endif
    }

    if (cfg_save(&cfg) != 0) { cfg_free(&cfg); return MCPKG_ERROR_FS; }
    cfg_free(&cfg);
    return MCPKG_ERROR_SUCCESS;
}

#endif /* MCPKG_CONFIG_H */
