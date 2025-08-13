#ifndef MCPKG_CONFIG_H
#define MCPKG_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mcpkg.h"
#include "utils/mcpkg_fs.h"
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

typedef struct {
	char *mc_base;
	char *mc_version;
	char *mc_loader;
} McPkgConfig;


static void mcpkg_cfg_free(McPkgConfig *cfg)
{
	if (!cfg)
		return;

	free(cfg->mc_base);
	free(cfg->mc_version);
	free(cfg->mc_loader);
	memset(cfg, 0, sizeof(*cfg));
}

static MCPKG_ERROR_TYPE mcpkg_cfg_load(McPkgConfig *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	char *path = mcpkg_fs_config_file();
	if (!path)
		return MCPKG_ERROR_OOM;

	FILE *fp = fopen(path, "r");
	if (!fp) {
		free(path);
		return MCPKG_ERROR_OOM;
	}

	char line[4096];
	while (fgets(line, sizeof(line), fp)) {
		size_t n = strlen(line);
		while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
			line[--n] = '\0';

		// comments
		if (!n || line[0] == '#')
			continue;


		char *eq = strchr(line, '=');
		if (!eq)
			continue;

		*eq = '\0';
		const char *key = line;
		const char *val = eq + 1;

		if (strcmp(key, "mc_base") == 0)
			cfg->mc_base = strdup(val);
		else if (strcmp(key, "mc_version") == 0)
			cfg->mc_version = strdup(val);
		else if (strcmp(key, "mc_loader") == 0)
			cfg->mc_loader  = strdup(val);
	}
	fclose(fp);
	free(path);
	return MCPKG_ERROR_SUCCESS;
}

static MCPKG_ERROR_TYPE mcpkg_cfg_save(const McPkgConfig *cfg)
{
	char *dir = mcpkg_fs_config_dir();
	if (!dir)
		return MCPKG_ERROR_FS;

	if (mcpkg_fs_dir_exists(dir) != MCPKG_ERROR_SUCCESS) {
		if (mcpkg_fs_mkdir(dir) != MCPKG_ERROR_SUCCESS) {
			free(dir);
			return MCPKG_ERROR_FS;
		}
	}

	free(dir);

	char *path = mcpkg_fs_config_file();
	if (!path)
		return MCPKG_ERROR_OOM;

	FILE *fp = fopen(path, "w");
	if (!fp) {
		free(path);
		return MCPKG_ERROR_FS;
	}

	if (cfg->mc_base)
		fprintf(fp, "mc_base=%s\n", cfg->mc_base);

	if (cfg->mc_version)
		fprintf(fp, "mc_version=%s\n", cfg->mc_version);

	if (cfg->mc_loader)
		fprintf(fp, "mc_loader=%s\n", cfg->mc_loader);

	int ok = (fclose(fp) == 0) ? 0 : -1;
	free(path);
	return ok;
}

static inline MCPKG_ERROR_TYPE mcpkg_cfg_mc_base(const char *mc_base)
{
	if (!mc_base || !*mc_base)
		return MCPKG_ERROR_OOM;

	McPkgConfig cfg;
	if (mcpkg_cfg_load(&cfg) != MCPKG_ERROR_SUCCESS)
		return MCPKG_ERROR_FS;

	free(cfg.mc_base);
	cfg.mc_base = strdup(mc_base);
	if (!cfg.mc_base && *mc_base) {
		mcpkg_cfg_free(&cfg);
		return MCPKG_ERROR_OOM;
	}

	int rc = mcpkg_cfg_save(&cfg);
	mcpkg_cfg_free(&cfg);

	return (rc == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

static inline MCPKG_ERROR_TYPE mcpkg_cfg_mc_version(const char *mc_version)
{
	if (!mc_version || !*mc_version)
		return MCPKG_ERROR_PARSE;

	McPkgConfig cfg;
	if (mcpkg_cfg_load(&cfg) != 0)
		return MCPKG_ERROR_FS;

	free(cfg.mc_version);
	cfg.mc_version = strdup(mc_version);
	if (!cfg.mc_version && *mc_version) {
		mcpkg_cfg_free(&cfg);
		return MCPKG_ERROR_OOM;
	}

	int rc = mcpkg_cfg_save(&cfg);
	mcpkg_cfg_free(&cfg);
	return (rc == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

static inline MCPKG_ERROR_TYPE mcpkg_cfg_mc_loader(const char *mc_loader)
{
	if (!mc_loader || !*mc_loader)
		return MCPKG_ERROR_PARSE;

	McPkgConfig cfg;
	if (mcpkg_cfg_load(&cfg) != 0)
		return MCPKG_ERROR_FS;

	free(cfg.mc_loader);
	cfg.mc_loader = strdup(mc_loader);
	if (!cfg.mc_loader && *mc_loader) {
		mcpkg_cfg_free(&cfg);
		return MCPKG_ERROR_OOM;
	}

	int rc = mcpkg_cfg_save(&cfg);
	mcpkg_cfg_free(&cfg);
	return (rc == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

static inline MCPKG_ERROR_TYPE mcpkg_cfg_init(void)
{
	McPkgConfig cfg;
	if (mcpkg_cfg_load(&cfg) != 0)
		return MCPKG_ERROR_FS;

	if (!cfg.mc_base) {
#ifdef _WIN32
		const char *appdata = getenv("APPDATA");
		if (appdata) {
			if (asprintf(&cfg.mc_base, "%s/minecraft", appdata) < 0) {
				mcpkg_cfg_free(&cfg);
				return MCPKG_ERROR_OOM;
			}
		}
#else
		const char *home = getenv("HOME");
		if (home) {
			if (asprintf(&cfg.mc_base, "%s/.minecraft", home) < 0) {
				mcpkg_cfg_free(&cfg);
				return MCPKG_ERROR_OOM;
			}
		}
#endif
	}

	if (mcpkg_cfg_save(&cfg) != 0) {
		mcpkg_cfg_free(&cfg);
		return MCPKG_ERROR_FS;
	}
	mcpkg_cfg_free(&cfg);
	return MCPKG_ERROR_SUCCESS;
}

MCPKG_END_DECLS
#endif // MCPKG_CONFIG_H
