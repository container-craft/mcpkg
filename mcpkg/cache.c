#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mcpkg.h>
#include <mcpkg_cache.h>

int search_cache_command(const char *mc_version, const char *mod_loader,
                         const char *package_name)
{
	McPkgCache *cache = mcpkg_cache_new();
	if (!cache) {
		fprintf(stderr, "Failed to initialize cache.\n");
		return MCPKG_ERROR_GENERAL;
	}

	int result = mcpkg_cache_load(cache, mod_loader, mc_version);
	if (result != MCPKG_ERROR_SUCCESS) {
		fprintf(stderr, "Failed to load cache. Error code: %d\n", result);
		mcpkg_cache_free(cache);
		return result;
	}

	size_t matches_count;
	McPkgInfoEntry **matches = mcpkg_cache_search(cache, package_name,
	                           &matches_count);

	if (matches) {
		for (size_t i = 0; i < matches_count; ++i)
			printf("%s - %s\n", matches[i]->name, matches[i]->description);
		free(matches);
	} else {
		printf("No matches found for '%s'.\n", package_name);
	}

	mcpkg_cache_free(cache);
	return MCPKG_ERROR_SUCCESS;
}

int show_cache_command(const char *mc_version, const char *mod_loader,
                       const char *package_name)
{
	McPkgCache *cache = mcpkg_cache_new();
	if (!cache) {
		fprintf(stderr, "Failed to initialize cache.\n");
		return MCPKG_ERROR_GENERAL;
	}

	int result = mcpkg_cache_load(cache, mod_loader, mc_version);
	if (result != MCPKG_ERROR_SUCCESS) {
		fprintf(stderr, "Failed to load cache. Error code: %d\n", result);
		mcpkg_cache_free(cache);
		return result;
	}

	char *mod_string = mcpkg_cache_show(cache, package_name);
	if (mod_string && strlen(mod_string) > 0) {
		printf("Details for '%s':\n", package_name);
		printf("%s\n", mod_string);
		free(mod_string);
	} else {
		printf("Package '%s' not found in cache.\n", package_name);
	}

	mcpkg_cache_free(cache);
	return MCPKG_ERROR_SUCCESS;
}
