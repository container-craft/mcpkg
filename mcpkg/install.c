#include "install.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mcpkg.h>
#include <utils/array_helper.h>
#include <api/modrith_client.h>

mcpkg_error_types install_command(const char *mc_version, const char *mod_loader, str_array *packages)
{
    if (!mc_version || !mod_loader || !packages || packages->count == 0) {
        fprintf(stderr, "install: invalid arguments\n");
        return MCPKG_ERROR_PARSE;
    }

    // Create a Modrinth client tied to this version/loader
    ModrithApiClient *client = modrith_client_new(mc_version, mod_loader);
    if (!client) {
        fprintf(stderr, "install: failed to create Modrinth client\n");
        return MCPKG_ERROR_OOM;
    }

    // Install each requested package
    int failures = 0;
    for (size_t i = 0; i < packages->count; ++i) {
        const char *pkg = packages->elements[i];
        if (!pkg || !*pkg) continue;

        printf("==> Installing %s for %s / %s\n", pkg, mod_loader, mc_version);
        int rc = modrith_client_install(client, pkg);
        if (rc != MCPKG_ERROR_SUCCESS) {
            fprintf(stderr, "   Failed to install %s (code %d)\n", pkg, rc);
            failures++;
        } else {
            printf("   Installed %s\n", pkg);
        }
    }

    modrith_client_free(client);
    return failures ? MCPKG_ERROR_GENERAL : MCPKG_ERROR_SUCCESS;
}

mcpkg_error_types rm_command(const char *mc_version, const char *mod_loader, str_array *packages)
{
    (void)mc_version;
    (void)mod_loader;
    (void)packages;
    printf("remove: not supported yet\n");
    return MCPKG_ERROR_GENERAL;
}
