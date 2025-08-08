#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcpkg.h"
#include "update.h"

int main(int argc, char *argv[]) {
    // Simple command-line argument parsing for the "update" command
    if (argc < 2 || strcmp(argv[1], "update") != 0) {
        fprintf(stderr, "Usage: %s update [options]\n", argv[0]);
        return 1;
    }

    // Default values
    const char *default_mc_version = "1.21.8";
    const char *default_mod_loader = "fabric";

    // Use environment variables if available
    const char *mc_version = getenv(ENV_MC_VERSION);
    if (!mc_version) {
        mc_version = default_mc_version;
    }

    const char *mod_loader = getenv(ENV_MC_LOADER);
    if (!mod_loader) {
        mod_loader = default_mod_loader;
    }

    printf("Updating local cache for Minecraft version '%s' and loader '%s'...\n", mc_version, mod_loader);

    if (run_update_command(mc_version, mod_loader) != 0) {
        fprintf(stderr, "Failed to run update command.\n");
        return 1;
    }

    printf("Update complete.\n");

    return 0;
}
