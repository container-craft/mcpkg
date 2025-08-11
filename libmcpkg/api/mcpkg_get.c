#include "mcpkg_get.h"
#include <errno.h>
#include <unistd.h>


#include "api/modrith_client.h"
#include "utils/code_names.h"

// Simple download (sha verification TODO)
MCPKG_ERROR_TYPE mcpkg_get_download(ApiClient *api,
                       const char *url,
                       const char *sha,
                       const char *dest_path)
{
    (void)sha; // TODO: implement sha512 and others verify
    if (!api || !api->curl || !url || !dest_path)
        return MCPKG_ERROR_GENERAL;

    FILE *fp = fopen(dest_path, "wb");
    if (!fp)
        return MCPKG_ERROR_FS;

    curl_easy_setopt(api->curl, CURLOPT_URL, url);
    curl_easy_setopt(api->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(api->curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(api->curl, CURLOPT_USERAGENT, MCPKG_USER_AGENT);

    CURLcode res = curl_easy_perform(api->curl);
    fclose(fp);
    if (res != CURLE_OK) {
        unlink(dest_path);
        fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(res));
        return MCPKG_ERROR_NETWORK;
    }
    return MCPKG_ERROR_SUCCESS;
}


McPkgGet *mcpkg_get_new()
{
    McPkgGet *g = calloc(1, sizeof(McPkgGet));
    return g;
}

void mcpkg_get_free(McPkgGet *get)
{
    if (!get)
        return;

    free(get->base_path);
    if (get->mods) {
        for (size_t i = 0; i < get->mods_count; ++i) mcpkg_entry_free(get->mods[i]);
        free(get->mods);
    }

    free(get);
}


static const char *mcpkg_get_find_installed_version(const char *cache_root,
                                          const char *loader,
                                          const char *mcver,
                                          const char *codename,
                                          const char *id_or_slug)
{
    char *install_db = NULL;
    if (mcpkg_fs_db_dir(&install_db,
                        cache_root,
                        loader,
                        codename, mcver) != 0)
        return NULL;

    McPkgEntry **installed = NULL; size_t installed_count = 0;
    (void)mcpkg_get_db(install_db, &installed, &installed_count);
    free(install_db);

    const char *found = NULL;
    for (size_t i = 0; i < installed_count; ++i) {
        McPkgEntry *e = installed[i];
        if (!e)
            continue;
        if ((e->name && strcmp(e->name, id_or_slug) == 0) ||
            (e->id   && strcmp(e->id,   id_or_slug) == 0)) {
            found = e->version; // MEMEORY: returning pointer owned by e
            break;
        }
    }

    // We can’t return a pointer owned by installed[] after freeing it, so clone if found.
    char *ret = NULL;
    if (found)
        ret = strdup(found);

    if (installed) {
        for (size_t i = 0; i < installed_count; ++i) mcpkg_entry_free(installed[i]);
        free(installed);
    }
    return ret; // caller must free
}

MCPKG_ERROR_TYPE mcpkg_get_install(const char *mc_version,
                      const char *mod_loader,
                      str_array *packages)
{
    if (!mc_version || !mod_loader || !packages || packages->count == 0) {
        fprintf(stderr, "install: invalid arguments\n");
        return MCPKG_ERROR_PARSE;
    }

    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root) cache_root = MCPKG_CACHE;

    const char *codename = codename_for_version(mc_version);
    if (!codename) {
        fprintf(stderr, "install: unknown codename for version %s\n", mc_version);
        return MCPKG_ERROR_VERSION_MISMATCH;
    }

    // Create a Modrinth client tied to this version/loader
    ModrithApiClient *client = modrith_client_new(mc_version, mcpkg_modloader_from_str(mod_loader));
    if (!client) {
        fprintf(stderr, "install: failed to create Modrinth client\n");
        return MCPKG_ERROR_OOM;
    }

    int failures = 0, skipped = 0, installed_ok = 0;

    for (size_t i = 0; i < packages->count; ++i) {
        const char *pkg = packages->elements[i];
        if (!pkg || !*pkg)
            continue;

        // Check if already installed (same version)
        char *installed_ver = (char *)mcpkg_get_find_installed_version(cache_root, mod_loader, mc_version, codename, pkg);
        if (installed_ver) {
            // Ask Modrinth for the best candidate to compare; if same, skip
            cJSON *versions = modrith_get_versions_json(client, pkg);
            if (!versions) {
                fprintf(stderr, "   %s: failed to query versions; proceeding to install anyway\n", pkg);
                // fall through to install
            } else {
                cJSON *best = modrith_pick_best_version(client, versions);
                if (best) {
                    McPkgEntry *tmp = NULL;
                    if (modrith_version_to_entry(best, &tmp) == 0 && tmp && tmp->version) {
                        if (strcmp(tmp->version, installed_ver) == 0) {
                            printf("%s: already at latest (%s) — skipping\n", pkg, installed_ver);
                            skipped++;
                            mcpkg_entry_free(tmp);
                            cJSON_Delete(versions);
                            free(installed_ver);
                            continue; // next package
                        }
                    }
                    if (tmp)
                        mcpkg_entry_free(tmp);
                }
                cJSON_Delete(versions);
            }
        }
        free(installed_ver);

        printf("Installing %s for %s / %s\n", pkg, mod_loader, mc_version);
        int rc = modrith_client_install(client, pkg);
        if (rc != MCPKG_ERROR_SUCCESS) {
            fprintf(stderr, "   Failed to install %s (code %d)\n", pkg, rc);
            failures++;
        } else {
            installed_ok++;
            printf("Installed %s\n", pkg);
        }
    }

    modrith_client_free(client);

    if (failures) {
        printf("Summary: %d installed, %d skipped (up-to-date), %d failed\n",
               installed_ok, skipped, failures);
        return MCPKG_ERROR_GENERAL;
    }
    printf("Summary: %d installed, %d skipped (up-to-date), 0 failed\n",
           installed_ok, skipped);
    return MCPKG_ERROR_SUCCESS;
}

int mcpkg_get_remove(const char *mc_version, const char *mod_loader, str_array *packages)
{
    if (!mc_version || !mod_loader || !packages || packages->count == 0)
        return MCPKG_ERROR_PARSE;

    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root) cache_root = MCPKG_CACHE;

    const char *codename = codename_for_version(mc_version);
    if (!codename) return MCPKG_ERROR_VERSION_MISMATCH;

    // Resolve paths
    char *mods_dir = NULL;
    if (mcpkg_fs_mods_dir(&mods_dir,
                          cache_root,
                          mod_loader,
                          codename, mc_version) != 0)
        return MCPKG_ERROR_OOM;

    char *install_db = NULL;
    if (mcpkg_fs_db_dir(&install_db,
                        cache_root,
                        mod_loader,
                        codename, mc_version) != 0) {
        free(mods_dir);
        return MCPKG_ERROR_OOM;
    }

    // If there’s no install DB, nothing to remove
    if (access(install_db, F_OK) != 0) {
        free(mods_dir);
        free(install_db);
        return MCPKG_ERROR_SUCCESS;
    }

    // Load current DB
    McPkgEntry **entries = NULL; size_t count = 0;
    if (mcpkg_get_db(install_db, &entries, &count) != MCPKG_ERROR_SUCCESS) {
        free(mods_dir);
        free(install_db);
        return MCPKG_ERROR_FS;
    }

    // For each requested package, find matching entries (by slug or id), remove jar(s) and mark entry for deletion
    size_t kept = 0;
    for (size_t i = 0; i < count; ++i) {
        McPkgEntry *e = entries[i];
        if (!e)
            continue;

        int should_delete = 0;
        for (size_t p = 0; p < packages->count; ++p) {
            const char *q = packages->elements[p];
            if (!q)
                continue;
            if ((e->name && strcmp(e->name, q) == 0) ||
                (e->id   && strcmp(e->id,   q) == 0)) {
                should_delete = 1;
                break;
            }
        }

        if (!should_delete) {
            // keep entry, compact into place if needed
            entries[kept++] = e;
            continue;
        }

        // Remove JAR if present
        if (e->file_name && *e->file_name) {
            char *jar_path = NULL;
            if (asprintf(&jar_path, "%s/%s", mods_dir, e->file_name) >= 0 && jar_path) {
                if (access(jar_path, F_OK) == 0) {
                    if (unlink(jar_path) != 0) {
                        fprintf(stderr, "warning: failed to remove %s: %s\n", jar_path, strerror(errno));
                    } else {
                        printf("Removed %s\n", jar_path);
                    }
                }
                free(jar_path);
            }
        }

        // free the entry (we’re removing it from DB)
        mcpkg_entry_free(e);
        entries[i] = NULL;
    }

    // If nothing left, write an empty file (or unlink — your choice)
    int rc = 0;
    if (kept == 0) {
        // Option A: truncate to empty file
        FILE *fp = fopen(install_db, "wb");
        if (!fp) {
            perror("truncate Packages.install");
            rc = MCPKG_ERROR_FS;
        } else {
            fclose(fp);
        }
        // Option B (alternative): unlink(install_db);
    } else {
        // Write kept entries back
        rc = mcpkg_get_db_write_all(install_db, entries, kept) == 0 ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
    }

    // Free the compacted entries array
    // Only the first `kept` entries are still valid pointers; the rest were freed above.
    for (size_t i = 0; i < kept; ++i) {
        // These are still owned by DB; if you prefer to not keep them around, free them now:
        // But since we just wrote them to disk and we’re returning, free to avoid leaks:
        mcpkg_entry_free(entries[i]);
    }
    free(entries);
    free(mods_dir);
    free(install_db);

    return rc;
}


/* Append formatted text to a growable heap buffer.
 * On success returns 0, and *buf / *len / *cap are updated.
 * On OOM or vsnprintf error returns -1 (buffer is unchanged on OOM).
 */
static int buf_appendf(char **buf, size_t *len, size_t *cap, const char *fmt, ...) {
    va_list ap;
    while (1) {
        va_start(ap, fmt);
        int need = vsnprintf((*buf) ? *buf + *len : NULL, (*buf && *cap > *len) ? *cap - *len : 0, fmt, ap);
        va_end(ap);
        if (need < 0)
            return -1;

        size_t need_sz = (size_t)need;
        if (*buf && *len + need_sz + 1 <= *cap) {     // fits
            *len += need_sz;
            return 0;
        }

        // grow
        size_t newcap = (*cap ? *cap * 2 : 512);
        size_t mincap = *len + need_sz + 1;
        if (newcap < mincap)
            newcap = mincap;

        char *tmp = (char *)realloc(*buf, newcap);
        if (!tmp)
            return -1;                          // OOM, leave original intact

        *buf = tmp;
        *cap = newcap;
        // FIXME
        // loop to actually write into the newly grown buffer
    }
}

char *mcpkg_get_policy(const char *mc_version,
                       const char *mod_loader,
                       str_array *packages)
{
    if (!mc_version || !mod_loader || !packages)
        return strdup("(invalid arguments)");

    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root)
        cache_root = MCPKG_CACHE;

    const char *codename = codename_for_version(mc_version);
    if (!codename)
        return strdup("(unknown Minecraft version codename)");

    char *install_db = NULL;
    if (mcpkg_fs_db_dir(&install_db,
                        cache_root,
                        mod_loader,
                        codename, mc_version) != 0)
        return strdup("(oom)");

    McPkgEntry **installed = NULL;
    size_t installed_count = 0;
    (void)mcpkg_get_db(install_db, &installed, &installed_count);

    // Prepare Modrinth API client for candidate version lookups
    ModrithApiClient *client = modrith_client_new(mc_version, mcpkg_modloader_from_str(mod_loader));
    if (!client) {
        free(install_db);
        return strdup("(failed to init API client)");
    }

    char *report = NULL;
    size_t cap = 0, len = 0;

    for (size_t i = 0; i < packages->count; ++i) {
        const char *pkg = packages->elements[i];
        const char *installed_ver = NULL;
        const char *candidate_ver = NULL;

        // Look up installed version in DB
        for (size_t j = 0; j < installed_count; ++j) {
            McPkgEntry *e = installed[j];
            if ((e->name && strcmp(e->name, pkg) == 0) ||
                (e->id && strcmp(e->id, pkg) == 0)) {
                installed_ver = e->version;
                break;
            }
        }

        // Query Modrinth for latest candidate
        cJSON *versions = modrith_get_versions_json(client, pkg);
        if (versions) {
            cJSON *best = modrith_pick_best_version(client, versions);
            if (best) {
                cJSON *ver_str = cJSON_GetObjectItemCaseSensitive(best, "version_number");
                if (cJSON_IsString(ver_str))
                    candidate_ver = ver_str->valuestring;
            }
            cJSON_Delete(versions);
        }

        if (buf_appendf(&report, &len, &cap, "%s:\n", pkg) < 0)
            goto oom;
        if (buf_appendf(&report, &len, &cap, "  Installed: %s\n",
                        installed_ver ? installed_ver : "None") < 0)
            goto oom;
        if (buf_appendf(&report, &len, &cap, "  Candidate: %s\n",
                        candidate_ver ? candidate_ver : "Unknown") < 0)
            goto oom;
    }

    // cleanup
    if (installed) {
        for (size_t j = 0; j < installed_count; ++j)
            mcpkg_entry_free(installed[j]);
        free(installed);
    }
    free(install_db);
    modrith_client_free(client);
    return report;

oom:
    if (installed) {
        for (size_t j = 0; j < installed_count; ++j)
            mcpkg_entry_free(installed[j]);
        free(installed);
    }
    free(install_db);
    modrith_client_free(client);
    free(report);
    return strdup("(oom)");
}

int mcpkg_get_upgreade(const char *mc_version,
                       const char *mod_loader)
{
    if (!mc_version || !mod_loader)
        return MCPKG_ERROR_PARSE;

    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root) cache_root = MCPKG_CACHE;

    const char *codename = codename_for_version(mc_version);
    if (!codename)
        return MCPKG_ERROR_VERSION_MISMATCH;

    // Paths
    char *install_db = NULL;
    if (mcpkg_fs_db_dir(&install_db, cache_root, mod_loader, codename, mc_version) != 0)
        return MCPKG_ERROR_OOM;

    // Load installed DB; if missing, nothing to do
    if (access(install_db, F_OK) != 0) {
        printf("No installed packages found for %s / %s.\n", mod_loader, mc_version);
        free(install_db);
        return MCPKG_ERROR_SUCCESS;
    }

    McPkgEntry **installed = NULL; size_t installed_count = 0;
    if (mcpkg_get_db(install_db, &installed, &installed_count) != 0) {
        free(install_db);
        return MCPKG_ERROR_FS;
    }

    if (installed_count == 0) {
        printf("No installed packages found for %s / %s.\n", mod_loader, mc_version);
        free(install_db);
        free(installed);
        return MCPKG_ERROR_SUCCESS;
    }

    // Modrinth client for lookups and installs
    ModrithApiClient *client = modrith_client_new(mc_version, mcpkg_modloader_from_str(mod_loader));
    if (!client) {
        for (size_t i = 0; i < installed_count; ++i) mcpkg_entry_free(installed[i]);
        free(installed);
        free(install_db);
        return MCPKG_ERROR_OOM;
    }

    int upgrades = 0, failures = 0;

    for (size_t i = 0; i < installed_count; ++i) {
        McPkgEntry *e = installed[i];
        if (!e)
            continue;

        // Prefer project id if available; otherwise slug (name)
        const char *identifier = e->id ? e->id : e->name;
        if (!identifier || !*identifier)
            continue;

        // Fetch candidate
        const char *candidate_ver = NULL;
        cJSON *versions = modrith_get_versions_json(client, identifier);
        if (versions) {
            cJSON *best = modrith_pick_best_version(client, versions);
            if (best) {
                cJSON *vn = cJSON_GetObjectItemCaseSensitive(best, "version_number");
                if (cJSON_IsString(vn)) candidate_ver = vn->valuestring;
            }
            cJSON_Delete(versions);
        }

        // If we can't determine a candidate, skip this one
        if (!candidate_ver || !*candidate_ver) {
            printf("%s: unable to determine candidate version; skipping\n",
                   identifier);
            continue;
        }

        // If already up to date, skip
        if (e->version && strcmp(e->version, candidate_ver) == 0) {
            printf("%s: up to date (%s)\n", identifier, candidate_ver);
            continue;
        }

        // Otherwise, upgrade
        printf("%s: upgrading %s -> %s\n",
               identifier,
               e->version ? e->version : "(none)",
               candidate_ver);

        int rc = modrith_client_install(client, identifier);
        if (rc == MCPKG_ERROR_SUCCESS) {
            upgrades++;
        } else {
            fprintf(stderr, "  failed to upgrade %s (code %d)\n", identifier, rc);
            failures++;
        }
    }

    for (size_t i = 0; i < installed_count; ++i)
        mcpkg_entry_free(installed[i]);

    free(installed);
    modrith_client_free(client);
    free(install_db);

    if (failures) {
        fprintf(stderr, "Completed with %d upgrade(s), %d failure(s).\n", upgrades, failures);
        return MCPKG_ERROR_GENERAL;
    }
    printf("Completed with %d upgrade(s).\n", upgrades);
    return MCPKG_ERROR_SUCCESS;
}
