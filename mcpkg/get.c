#include "get.h"

#include <stdio.h>
#include <stdlib.h>

#include <mcpkg.h>
#include <mcpkg_get.h>
#include <utils/array_helper.h>
#include <api/modrith_client.h>

MCPKG_ERROR_TYPE install_command(const char *mc_version, const char *mod_loader, str_array *packages)
{
    if (!mc_version || !mod_loader || !packages || packages->count == 0) {
        fprintf(stderr, "install: missing version/loader/packages\n");
        return MCPKG_ERROR_PARSE;
    }

    MCPKG_ERROR_TYPE rc = mcpkg_get_install(mc_version, mod_loader, packages);
    if (rc != MCPKG_ERROR_SUCCESS) {
        fprintf(stderr, "install: one or more installs failed (code %d)\n", rc);
        return rc;
    }
    return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE remove_command(const char *mc_version, const char *mod_loader, str_array *packages)
{
    if (!mc_version || !mod_loader || !packages || packages->count == 0) {
        fprintf(stderr, "remove: missing version/loader/packages\n");
        return MCPKG_ERROR_PARSE;
    }

    MCPKG_ERROR_TYPE rc = mcpkg_get_remove(mc_version, mod_loader, packages);
    if (rc != MCPKG_ERROR_SUCCESS) {
        fprintf(stderr, "remove: one or more removals failed (code %d)\n", rc);
        return rc;
    }
    return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE policy_command(const char *mc_version, const char *mod_loader, str_array *packages)
{
    if (!mc_version || !mod_loader || !packages || packages->count == 0) {
        fprintf(stderr, "policy: missing version/loader/packages\n");
        return MCPKG_ERROR_PARSE;
    }

    char *report = mcpkg_get_policy(mc_version, mod_loader, packages);
    if (!report) {
        fprintf(stderr, "policy: failed to generate policy report\n");
        return MCPKG_ERROR_GENERAL;
    }

    // Print the policy report exactly as requested
    fputs(report, stdout);
    free(report);
    return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE upgrade_command(const char *mc_version, const char *mod_loader)
{
    if (!mc_version || !mod_loader) {
        fprintf(stderr, "Error: missing Minecraft version or loader.\n");
        return MCPKG_ERROR_PARSE;
    }

    int rc = mcpkg_get_upgreade(mc_version, mod_loader);
    if (rc != MCPKG_ERROR_SUCCESS) {
        fprintf(stderr, "Upgrade failed with error code %d\n", rc);
        return rc;
    }

    printf("All packages upgraded successfully.\n");
    return MCPKG_ERROR_SUCCESS;
}
