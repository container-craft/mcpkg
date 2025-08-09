#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <mcpkg_cache.h>
#include <mcpkg.h>
#include <utils/array_helper.h>
//local
#include "config.h"
#include "update.h"
#include "cache.h"
#include "get.h"
#include "activate.h"

// Command-line options
static struct option long_options[] = {
    {"version", required_argument, 0, 'v'},
    {"loader", required_argument, 0, 'l'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

static void print_help(const char *prog_name) {
    printf("Welcome to McPkg a Minecraft package manager:\n");
    printf("Global Options:\n");
    printf("  -v, --version <version>    Minecraft version (e.g., 1.21.8)\n");
    printf("  -l, --loader <loader>      Mod loader (e.g., fabric)\n");
    printf("  -h, --help                 Show this help message\n");
    printf("Commands:\n");
    printf("  update                     Update the local package cache\n");
    printf("  upgrade                    Upgrades to newest package for the modloader at a version\n");
    printf("  cache                      Interact with the local cache\n");
    printf("  install                    Installs a mod for a loader at a version\n");
    printf("  remove                     Removes a mod for a loader at a version\n");
    printf("  policy                     Shows installed version vs available \n");
    printf("  global                     set the mincraft root folder, version, modloader");
    printf("\nExample Usage: %s update --version 1.21.6 -l forge\n", prog_name);
}

static void print_cache_help() {
    printf("mcpkg Cache\n");
    printf("Commands\n");
    printf(" * show    <package>    Search information about package from cache\n");
    printf(" * search  <package>    Search the local cache for a package\n");
    printf("\nExample Usage: mcpkg cache search sodium\n");
}

static void print_install_help() {
    printf("mcpkg Install\n");
    printf("Usage:\n");
    printf("  mcpkg install <package> [<package> ...]\n");
    printf("\nExamples:\n");
    printf("  mcpkg install sodium\n");
    printf("  mcpkg install sodium tweakeroo lithium\n");
}

static void print_global_help() {
    printf("mcpkg configuration\n");
    printf(" These configs are meant to match what your client(minecraft launcher uses) \n");
    printf("Commands\n");
    printf(" * mcbase     <Path>       Path to where minecradt is installed. Server or Client.\n");
    printf(" * loader     <loader>     Set the global mod loader.\n");
    printf(" * version    <version>    Set the global version.\n");
}

int main(int argc, char *argv[]) {
    const char *mc_version = getenv(ENV_MC_VERSION) ? getenv(ENV_MC_VERSION) : "1.21.8";
    const char *mod_loader = getenv(ENV_MC_LOADER) ? getenv(ENV_MC_LOADER) : "fabric";
    int opt;

    // Parse global options first
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "v:l:h", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'v': mc_version = optarg; break;
        case 'l': mod_loader = optarg; break;
        case 'h': print_help(argv[0]); return 0;
        default:  return 1;
        }
    }

    if (optind >= argc) {
        print_help(argv[0]);
        return 1;
    }

    const char *command = argv[optind];

    if (strcmp(command, "update") == 0) {
        return run_update_command(mc_version, mod_loader);

    } else if (strcmp(command, "upgrade") == 0) {
        return upgrade_command(mc_version, mod_loader);

    } else if (strcmp(command, "install") == 0) {
        // Remaining args after "install" are package names
        if (optind + 1 >= argc) {
            fprintf(stderr, "Error: 'install' requires at least one package name.\n");
            print_install_help();
            return 1;
        }

        str_array *packages = str_array_new();
        if (!packages) {
            fprintf(stderr, "Error: out of memory.\n");
            return 1;
        }
        for (int i = optind + 1; i < argc; ++i) {
            if (argv[i] && *argv[i]) (void)str_array_add(packages, argv[i]);
        }

        mcpkg_error_types rc = install_command(mc_version, mod_loader, packages);
        str_array_free(packages);
        return rc;

    } else if (strcmp(command, "remove") == 0) {
        // Remaining args after "remove" are package names
        if (optind + 1 >= argc) {
            fprintf(stderr, "Error: 'remove' requires at least one package name.\n");
            printf("Usage: %s remove <package> [<package> ...]\n", argv[0]);
            return 1;
        }

        str_array *packages = str_array_new();
        if (!packages) {
            fprintf(stderr, "Error: out of memory.\n");
            return 1;
        }
        for (int i = optind + 1; i < argc; ++i) {
            if (argv[i] && *argv[i])
                (void)str_array_add(packages, argv[i]);
        }

        mcpkg_error_types rc = remove_command(mc_version, mod_loader, packages);
        str_array_free(packages);
        return rc;

    } else if (strcmp(command, "policy") == 0) {
        // Remaining args after "policy" are package names
        if (optind + 1 >= argc) {
            fprintf(stderr, "Error: 'policy' requires at least one package name.\n");
            printf("Usage: %s policy <package> [<package> ...]\n", argv[0]);
            return 1;
        }

        str_array *packages = str_array_new();
        if (!packages) {
            fprintf(stderr, "Error: out of memory.\n");
            return 1;
        }
        for (int i = optind + 1; i < argc; ++i) {
            if (argv[i] && *argv[i]) (void)str_array_add(packages, argv[i]);
        }

        mcpkg_error_types rc = policy_command(mc_version, mod_loader, packages);
        str_array_free(packages);
        return rc;

    } else if (strcmp(command, "cache") == 0) {
        if (optind + 1 >= argc) {
            print_cache_help();
            return 1;
        }

        const char *cache_command = argv[optind + 1];
        const char *package_name  = (optind + 2 < argc) ? argv[optind + 2] : NULL;

        if (strcmp(cache_command, "search") == 0) {
            if (!package_name) {
                fprintf(stderr, "Error: 'cache search' requires a package name.\n");
                print_cache_help();
                return 1;
            }
            return search_cache_command(mc_version, mod_loader, package_name);

        } else if (strcmp(cache_command, "show") == 0) {
            if (!package_name) {
                fprintf(stderr, "Error: 'cache show' requires a package name.\n");
                print_cache_help();
                return 1;
            }
            return show_cache_command(mc_version, mod_loader, package_name);

        } else {
            print_cache_help();
            return 1;
        }

    } else if (strcmp(command, "global") == 0) {
        if (optind + 1 >= argc) {
            print_global_help();
            return 1;
        }

        const char *sub = argv[optind + 1];
        const char *val = (optind + 2 < argc) ? argv[optind + 2] : NULL;

        if (!sub || !*sub) {
            print_global_help();
            return 1;
        }

        if (strcmp(sub, "mcbase") == 0) {
            if (!val) {
                fprintf(stderr, "Error: 'global mcbase' requires a path.\n");
                return 1;
            }
            mcpkg_error_types rc = set_global_mc_base(val);
            if (rc != MCPKG_ERROR_SUCCESS) return rc;

            // Re-activate using current CLI/env version+loader
            mcpkg_error_types arc = mcpkg_activate(mc_version, mod_loader);
            if (arc != MCPKG_ERROR_SUCCESS) {
                fprintf(stderr, "Warning: set mcbase OK, but activate failed (%d)\n", arc);
            }
            return arc;

        } else if (strcmp(sub, "loader") == 0) {
            if (!val) {
                fprintf(stderr, "Error: 'global loader' requires a loader name.\n");
                return 1;
            }
            mcpkg_error_types rc = set_global_mc_loader(val);
            if (rc != MCPKG_ERROR_SUCCESS) return rc;

            // Re-activate using NEW loader + current version
            mcpkg_error_types arc = mcpkg_activate(mc_version, val);
            if (arc != MCPKG_ERROR_SUCCESS) {
                fprintf(stderr, "Warning: set loader OK, but activate failed (%d)\n", arc);
            }
            return arc;

        } else if (strcmp(sub, "version") == 0) {
            if (!val) {
                fprintf(stderr, "Error: 'global version' requires a Minecraft version.\n");
                return 1;
            }
            mcpkg_error_types rc = set_global_mc_version(val);
            if (rc != MCPKG_ERROR_SUCCESS) return rc;

            // Re-activate using NEW version + current loader
            mcpkg_error_types arc = mcpkg_activate(val, mod_loader);
            if (arc != MCPKG_ERROR_SUCCESS) {
                fprintf(stderr, "Warning: set version OK, but activate failed (%d)\n", arc);
            }
            return arc;

        } else {
            fprintf(stderr, "Unknown global subcommand: %s\n", sub);
            print_global_help();
            return 1;
        }

    } else {
        print_help(argv[0]);
        return 1;
    }

    // not reached
    return 0;
}
