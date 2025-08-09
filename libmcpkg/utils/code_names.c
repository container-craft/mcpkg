#include <string.h>
#include <stdio.h>
#include "code_names.h"

// A static array for each list of versions to ensure compile-time constant initializers.
static const char *tricky_trials_versions[] = {"1.21.8", "1.21.7", "1.21.6", "1.21.5", "1.21.4", "1.21.3", "1.21.2", "1.21.1", "1.21"};
static const char *trails_and_tales_versions[] = {"1.20.6", "1.20.5", "1.20.4", "1.20.3", "1.20.2", "1.20.1", "1.20"};
static const char *the_wild_versions[] = {"1.19.4", "1.19.3", "1.19.2", "1.19.1", "1.19"};
static const char *caves_and_cliffs_two_versions[] = {"1.18.2", "1.18.1", "1.18"};
static const char *caves_and_cliffs_one_versions[] = {"1.17.1", "1.17"};
static const char *nether_update_versions[] = {"1.16.5", "1.16.4", "1.16.3", "1.16.2", "1.16.1", "1.16"};
static const char *buzzy_bees_versions[] = {"1.15.2", "1.15.1", "1.15"};
static const char *village_and_pillage_versions[] = {"1.14.4", "1.14.3", "1.14.2", "1.14.1", "1.14"};
static const char *aquatic_versions[] = {"1.13.2", "1.13.1", "1.13"};
static const char *world_of_color_versions[] = {"1.12.2", "1.12.1", "1.12"};
static const char *exploration_versions[] = {"1.11.2", "1.11.1", "1.11"};
static const char *frostburn_versions[] = {"1.10.2", "1.10.1", "1.10"};
static const char *combat_versions[] = {"1.9"};
static const char *bountiful_versions[] = {"1.8.8", "1.8.7", "1.8.6", "1.8.5", "1.8.4", "1.8.3", "1.8.2", "1.8.1", "1.8"};
static const char *changed_the_world_versions[] = {"1.7.10", "1.7.9", "1.7.8", "1.7.7", "1.7.6", "1.7.5", "1.7.4", "1.7.2"};
static const char *horse_versions[] = {"1.6.4", "1.6.2", "1.6.1"};
static const char *redstone_versions[] = {"1.5.2", "1.5.1", "1.5"};
static const char *pretty_scary_versions[] = {"1.4.7", "1.4.6", "1.4.5", "1.4.4", "1.4.3", "1.4.2"};
static const char *villager_trading_versions[] = {"1.3.2", "1.3.1"};
static const char *faithful_versions[] = {"1.2.5", "1.2.4", "1.2.3", "1.2.2", "1.2.1"};
static const char *spawn_egg_versions[] = {"1.1"};
static const char *adventure_versions[] = {"1.0.1", "1.0"};

// In C, a mapping of codename to versions must be manually maintained.
// This is a static lookup table that points to the version arrays above.
const mc_versions_by_codename codename_to_versions_map[] = {
    {"tricky_trials", tricky_trials_versions, sizeof(tricky_trials_versions) / sizeof(tricky_trials_versions[0])},
    {"trails_and_tales", trails_and_tales_versions, sizeof(trails_and_tales_versions) / sizeof(trails_and_tales_versions[0])},
    {"the_wild", the_wild_versions, sizeof(the_wild_versions) / sizeof(the_wild_versions[0])},
    {"caves_and_cliffs_two", caves_and_cliffs_two_versions, sizeof(caves_and_cliffs_two_versions) / sizeof(caves_and_cliffs_two_versions[0])},
    {"caves_and_cliffs_one", caves_and_cliffs_one_versions, sizeof(caves_and_cliffs_one_versions) / sizeof(caves_and_cliffs_one_versions[0])},
    {"nether_update", nether_update_versions, sizeof(nether_update_versions) / sizeof(nether_update_versions[0])},
    {"buzzy_bees", buzzy_bees_versions, sizeof(buzzy_bees_versions) / sizeof(buzzy_bees_versions[0])},
    {"village_and_pillage", village_and_pillage_versions, sizeof(village_and_pillage_versions) / sizeof(village_and_pillage_versions[0])},
    {"aquatic", aquatic_versions, sizeof(aquatic_versions) / sizeof(aquatic_versions[0])},
    {"world_of_color", world_of_color_versions, sizeof(world_of_color_versions) / sizeof(world_of_color_versions[0])},
    {"exploration", exploration_versions, sizeof(exploration_versions) / sizeof(exploration_versions[0])},
    {"frostburn", frostburn_versions, sizeof(frostburn_versions) / sizeof(frostburn_versions[0])},
    {"combat", combat_versions, sizeof(combat_versions) / sizeof(combat_versions[0])},
    {"bountiful", bountiful_versions, sizeof(bountiful_versions) / sizeof(bountiful_versions[0])},
    {"changed_the_world", changed_the_world_versions, sizeof(changed_the_world_versions) / sizeof(changed_the_world_versions[0])},
    {"horse", horse_versions, sizeof(horse_versions) / sizeof(horse_versions[0])},
    {"redstone", redstone_versions, sizeof(redstone_versions) / sizeof(redstone_versions[0])},
    {"pretty_scary", pretty_scary_versions, sizeof(pretty_scary_versions) / sizeof(pretty_scary_versions[0])},
    {"villager_trading", villager_trading_versions, sizeof(villager_trading_versions) / sizeof(villager_trading_versions[0])},
    {"faithful", faithful_versions, sizeof(faithful_versions) / sizeof(faithful_versions[0])},
    {"spawn_egg", spawn_egg_versions, sizeof(spawn_egg_versions) / sizeof(spawn_egg_versions[0])},
    {"adventure", adventure_versions, sizeof(adventure_versions) / sizeof(adventure_versions[0])}
};

const size_t codename_to_versions_map_count = sizeof(codename_to_versions_map) / sizeof(mc_versions_by_codename);

// A static array of structs to map versions to codenames.
const mc_codename_map mc_codenames_map[] = {
    {"1.21.8", "tricky_trials"}, {"1.21.7", "tricky_trials"}, {"1.21.6", "tricky_trials"}, {"1.21.5", "tricky_trials"}, {"1.21.4", "tricky_trials"}, {"1.21.3", "tricky_trials"}, {"1.21.2", "tricky_trials"}, {"1.21.1", "tricky_trials"}, {"1.21", "tricky_trials"},
    {"1.20.6", "trails_and_tales"}, {"1.20.5", "trails_and_tales"}, {"1.20.4", "trails_and_tales"}, {"1.20.3", "trails_and_tales"}, {"1.20.2", "trails_and_tales"}, {"1.20.1", "trails_and_tales"}, {"1.20", "trails_and_tales"},
    {"1.19.4", "the_wild"}, {"1.19.3", "the_wild"}, {"1.19.2", "the_wild"}, {"1.19.1", "the_wild"}, {"1.19", "the_wild"},
    {"1.18.2", "caves_and_cliffs_two"}, {"1.18.1", "caves_and_cliffs_two"}, {"1.18", "caves_and_cliffs_two"},
    {"1.17.1", "caves_and_cliffs_one"}, {"1.17", "caves_and_cliffs_one"},
    {"1.16.5", "nether_update"}, {"1.16.4", "nether_update"}, {"1.16.3", "nether_update"}, {"1.16.2", "nether_update"}, {"1.16.1", "nether_update"}, {"1.16", "nether_update"},
    {"1.15.2", "buzzy_bees"}, {"1.15.1", "buzzy_bees"}, {"1.15", "buzzy_bees"},
    {"1.14.4", "village_and_pillage"}, {"1.14.3", "village_and_pillage"}, {"1.14.2", "village_and_pillage"}, {"1.14.1", "village_and_pillage"}, {"1.14", "village_and_pillage"},
    {"1.13.2", "aquatic"}, {"1.13.1", "aquatic"}, {"1.13", "aquatic"},
    {"1.12.2", "world_of_color"}, {"1.12.1", "world_of_color"}, {"1.12", "world_of_color"},
    {"1.11.2", "exploration"}, {"1.11.1", "exploration"}, {"1.11", "exploration"},
    {"1.10.2", "frostburn"}, {"1.10.1", "frostburn"}, {"1.10", "frostburn"},
    {"1.9", "combat"},
    {"1.8.8", "bountiful"}, {"1.8.7", "bountiful"}, {"1.8.6", "bountiful"}, {"1.8.5", "bountiful"}, {"1.8.4", "bountiful"}, {"1.8.3", "bountiful"}, {"1.8.2", "bountiful"}, {"1.8.1", "bountiful"}, {"1.8", "bountiful"},
    {"1.7.10", "changed_the_world"}, {"1.7.9", "changed_the_world"}, {"1.7.8", "changed_the_world"}, {"1.7.7", "changed_the_world"}, {"1.7.6", "changed_the_world"}, {"1.7.5", "changed_the_world"}, {"1.7.4", "changed_the_world"}, {"1.7.2", "changed_the_world"},
    {"1.6.4", "horse"}, {"1.6.2", "horse"}, {"1.6.1", "horse"},
    {"1.5.2", "redstone"}, {"1.5.1", "redstone"}, {"1.5", "redstone"},
    {"1.4.7", "pretty_scary"}, {"1.4.6", "pretty_scary"}, {"1.4.5", "pretty_scary"}, {"1.4.4", "pretty_scary"}, {"1.4.3", "pretty_scary"}, {"1.4.2", "pretty_scary"},
    {"1.3.2", "villager_trading"}, {"1.3.1", "villager_trading"},
    {"1.2.5", "faithful"}, {"1.2.4", "faithful"}, {"1.2.3", "faithful"}, {"1.2.2", "faithful"}, {"1.2.1", "faithful"},
    {"1.1", "spawn_egg"},
    {"1.0.1", "adventure"}, {"1.0", "adventure"}
};

const size_t mc_codenames_map_count = sizeof(mc_codenames_map) / sizeof(mc_codename_map);


const char *codename_for_version(const char *mc_version) {
    for (size_t i = 0; i < mc_codenames_map_count; ++i) {
        if (strcmp(mc_codenames_map[i].version, mc_version) == 0) {
            return mc_codenames_map[i].codename;
        }
    }
    return NULL;
}

const mc_versions_by_codename *versions_for_codename(const char *code_name) {
    for (size_t i = 0; i < codename_to_versions_map_count; ++i) {
        if (strcmp(codename_to_versions_map[i].codename, code_name) == 0) {
            return &codename_to_versions_map[i];
        }
    }
    return NULL;
}

// int compare_versions(const char *v1, const char *v2) {
//     int i1, i2, j1 = 0, j2 = 0, k1 = 0, k2 = 0;
//     // Parse version strings into parts
//     int count1 = sscanf(v1, "%d.%d.%d", &i1, &j1, &k1);
//     int count2 = sscanf(v2, "%d.%d.%d", &i2, &j2, &k2);
//     if (i1 != i2)
//         return i1 - i2;
//     if (count1 > 1 && count2 > 1 && j1 != j2)
//         return j1 - j2;
//     if (count1 > 2 && count2 > 2 && k1 != k2)
//         return k1 - k2;

//     return 0;
// }

const char *latest_version_for_codename(const char *code_name) {
    const mc_versions_by_codename *versions_struct = versions_for_codename(code_name);
    if (!versions_struct) {
        return NULL;
    }

    const char *latest = NULL;
    if (versions_struct->count > 0) {
        // The versions are already sorted from newest to oldest in the array
        latest = versions_struct->versions[0];
    }

    return latest;
}
