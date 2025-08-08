#include "update.h"
#include <stdio.h>
#include <api/modrith_client.h>
#include <unistd.h>

int run_update_command(const char *mc_version, const char *mod_loader) {
    // We only have the Modrinth client implemented for now
    ModrithApiClient *modrith_client = modrith_client_new(mc_version, mod_loader);
    if (!modrith_client) {
        fprintf(stderr, "Failed to create Modrinth API client.\n");
        return 1;
    }

    printf("Starting Modrinth update...\n");
    str_array *mods_list = modrith_client_update(modrith_client);

    if (mods_list) {
        printf("Successfully updated. Found %zu mods.\n", mods_list->count);
        str_array_free(mods_list);
    } else {
        fprintf(stderr, "Modrinth update failed.\n");
    }

    modrith_client_free(modrith_client);
    return mods_list ? 0 : 1;
}
