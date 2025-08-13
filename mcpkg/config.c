#include "config.h"
#include <stdio.h>
#include <mcpkg.h>
#include <mcpkg_config.h>

MCPKG_ERROR_TYPE set_global_mc_base(const char *mc_base)
{
	if (!mc_base || !*mc_base) {
		fprintf(stderr, "Error: empty mc_base.\n");
		return MCPKG_ERROR_PARSE;
	}
	MCPKG_ERROR_TYPE rc = mcpkg_cfg_mc_base(mc_base);
	if (rc == MCPKG_ERROR_SUCCESS) {
		printf("mc_base set to: %s\n", mc_base);
	} else {
		fprintf(stderr, "Failed to set mc_base (code %d)\n", rc);
	}
	return rc;
}

MCPKG_ERROR_TYPE set_global_mc_loader(const char *mc_loader)
{
	if (!mc_loader || !*mc_loader) {
		fprintf(stderr, "Error: empty mc_loader.\n");
		return MCPKG_ERROR_PARSE;
	}
	MCPKG_ERROR_TYPE rc = mcpkg_cfg_mc_loader(mc_loader);
	if (rc == MCPKG_ERROR_SUCCESS) {
		printf("mc_loader set to: %s\n", mc_loader);
	} else {
		fprintf(stderr, "Failed to set mc_loader (code %d)\n", rc);
	}
	return rc;
}

MCPKG_ERROR_TYPE set_global_mc_version(const char *mc_version)
{
	if (!mc_version || !*mc_version) {
		fprintf(stderr, "Error: empty mc_version.\n");
		return MCPKG_ERROR_PARSE;
	}
	MCPKG_ERROR_TYPE rc = mcpkg_cfg_mc_version(mc_version);
	if (rc == MCPKG_ERROR_SUCCESS) {
		printf("mc_version set to: %s\n", mc_version);
	} else {
		fprintf(stderr, "Failed to set mc_version (code %d)\n", rc);
	}
	return rc;
}
