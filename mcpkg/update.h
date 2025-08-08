#ifndef UPDATE_H
#define UPDATE_H

/**
 * @brief Runs the update command to fetch mod information from providers.
 *
 * This function initializes the necessary API clients, fetches mod data from
 * upstream providers, and processes the results.
 *
 * @param mc_version The Minecraft version to target.
 * @param mod_loader The mod loader to target.
 * @return 0 on success, non-zero on failure.
 */
int run_update_command(const char *mc_version, const char *mod_loader);

#endif // UPDATE_H
