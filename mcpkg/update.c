#include "update.h"
#include <stdio.h>
#include <unistd.h>

#include <mcpkg.h>
#include <api/modrith_client.h>
#include <utils/array_helper.h>

int run_update_command(const char *mc_version, const char *mod_loader) {
    ModrithApiClient *modrith_client = modrith_client_new(mc_version, mcpkg_modloader_from_str(mod_loader));
    if (!modrith_client) {
        fprintf(stderr, "Failed to create Modrinth API client.\n");
        return 1;
    }

    printf("Starting Modrinth update...\n");

    int result = modrith_client_update(modrith_client);

    if (result == MCPKG_ERROR_SUCCESS) {
        printf("Successfully updated.\n");
        return 0;
    } else {
        fprintf(stderr, "Modrinth update failed with error code: %d\n", result);
        return 1;
    }

    modrith_client_free(modrith_client);
}
