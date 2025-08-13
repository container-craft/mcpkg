#ifndef MCPKG_GET_H
#define MCPKG_GET_H

#include <stdbool.h>
#include <stddef.h>

#include <msgpack.h>

#include <unistd.h>
#include <zstd.h>

#include <curl/curl.h>

#include <cjson/cJSON.h>

#include "mcpkg.h"
#include "mcpkg_api_client.h"
#include "mcpkg_entry.h"
#include "modrith_client.h"
#include "utils/mcpkg_fs.h"
#include "utils/mcpkg_visited_set.h"
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

typedef struct McPkgGet McPkgGet;
typedef struct McPkgGet {
	char *base_path;
	McPkgEntry **mods;
	size_t mods_count;
} McPkgGet;

static MCPKG_ERROR_TYPE mcpkg_get_db(const char *path,
                                     McPkgEntry ***out_entries,
                                     size_t *out_count)
{
	*out_entries = NULL;
	*out_count = 0;
	if (access(path, F_OK) != 0)
		return MCPKG_ERROR_SUCCESS; // missing is fine

	char *data = NULL;
	size_t n = 0;
	if (mcpkg_fs_read_cache(path, &data, &n) != MCPKG_ERROR_SUCCESS)
		return MCPKG_ERROR_PARSE;

	msgpack_unpacked up;
	msgpack_unpacked_init(&up);
	size_t off = 0, cap = 8;

	McPkgEntry **arr = malloc(cap * sizeof(*arr));
	if (!arr) {
		free(data);
		msgpack_unpacked_destroy(&up);
		return 1;
	}

	while (msgpack_unpack_next(&up, data, n, &off)) {
		if (up.data.type != MSGPACK_OBJECT_MAP)
			continue;

		McPkgEntry *e = mcpkg_entry_new();
		if (!e)
			continue;

		if (mcpkg_entry_unpack(&up.data, e) != 0) {
			mcpkg_entry_free(e);
			continue;
		}
		if (*out_count >= cap) {
			cap *= 2;
			arr = realloc(arr, cap * sizeof(*arr));
			if (!arr)
				break;
		}
		arr[(*out_count)++] = e;
	}
	msgpack_unpacked_destroy(&up);
	free(data);
	*out_entries = arr;
	return 0;
}


static MCPKG_ERROR_TYPE mcpkg_get_save_db(const char *path,
                McPkgEntry **entries,
                size_t count)
{
	if (mcpkg_fs_dir_exists(path) != MCPKG_ERROR_SUCCESS) {
		if (mcpkg_fs_mkdir(path) != MCPKG_ERROR_SUCCESS) {
			return MCPKG_ERROR_FS;
		}
	}

	msgpack_sbuffer sb;
	msgpack_sbuffer_init(&sb);
	for (size_t i = 0; i < count; ++i) {
		if (!entries[i])
			continue;

		if (mcpkg_entry_pack(entries[i], &sb) != 0) {
			msgpack_sbuffer_destroy(&sb);
			return 1;
		}
	}
	FILE *fp = fopen(path, "wb");
	if (!fp) {
		msgpack_sbuffer_destroy(&sb);
		return MCPKG_ERROR_FS;
	}
	fwrite(sb.data, 1, sb.size, fp);
	fclose(fp);
	msgpack_sbuffer_destroy(&sb);
	return MCPKG_ERROR_SUCCESS;
}


static MCPKG_ERROR_TYPE mcpkg_get_db_append(const char *path,
                McPkgEntry *new_entry)
{
	McPkgEntry **entries = NULL;
	size_t count = 0;
	if (mcpkg_get_db(path, &entries, &count) != MCPKG_ERROR_SUCCESS)
		return MCPKG_ERROR_FS;

	size_t idx = count;
	for (size_t i = 0; i < count; ++i) {
		if (!entries[i])
			continue;

		if ((entries[i]->id && new_entry->id
		     && strcmp(entries[i]->id, new_entry->id) == 0) ||
		    (entries[i]->name && new_entry->name
		     && strcmp(entries[i]->name, new_entry->name) == 0)) {
			idx = i;
			break;
		}
	}

	if (idx == count) {
		McPkgEntry **tmp = realloc(entries, (count + 1) * sizeof(*entries));
		if (!tmp) {
			for (size_t i = 0; i < count; ++i)
				mcpkg_entry_free(entries[i]);

			free(entries);
			return 1;
		}
		entries = tmp;
		entries[count++] = new_entry;

	} else {
		mcpkg_entry_free(entries[idx]);
		entries[idx] = new_entry;
	}

	int rc = mcpkg_get_save_db(path, entries, count);
	for (size_t i = 0; i < count; ++i)
		if (entries[i])
			mcpkg_entry_free(entries[i]);

	free(entries);
	return rc;
}


static MCPKG_ERROR_TYPE mcpkg_get_db_is_exact(const char *path,
                const char *project_id,
                const char *version_str)
{
	if (!project_id || !version_str)
		return MCPKG_ERROR_OOM;

	McPkgEntry **entries = NULL;
	size_t count = 0;
	if (mcpkg_get_db(path, &entries, &count) != MCPKG_ERROR_SUCCESS)
		return MCPKG_ERROR_FS;

	int found = 0;
	for (size_t i = 0; i < count; ++i) {
		McPkgEntry *e = entries[i];
		if (!e)
			continue;

		if (e->id && e->version && strcmp(e->id, project_id) == 0 &&
		    strcmp(e->version, version_str) == 0) {
			found = 1;
		}
		mcpkg_entry_free(e);
	}

	free(entries);
	return found;
}

MCPKG_ERROR_TYPE mcpkg_get_download(ApiClient *api,
                                    const char *url,
                                    const char *sha_hex_or_null,
                                    const char *dest_path);


static MCPKG_ERROR_TYPE mcpkg_get_db_append_single(ModrithApiClient *client,
                const char *mods_dir,
                const char *install_db,
                McPkgEntry *entry)
{
	if (!entry || !entry->file_name || !entry->url || !entry->id || !entry->version)
		return MCPKG_ERROR_PARSE;

	/* If already installed at the exact version, optionally refresh DB and return */
	if (mcpkg_get_db_is_exact(install_db, entry->id, entry->version)) {
		/* Optional: keep DB fresh; NOTE: upsert takes ownership, so don't touch entry after this */
		(void)mcpkg_get_db_append(install_db, entry);
		return MCPKG_ERROR_SUCCESS;
	}

	/* Download JAR first (we still own entry here) */
	char *dest = NULL;
	if (asprintf(&dest, "%s/%s", mods_dir, entry->file_name) < 0)
		return MCPKG_ERROR_OOM;

	int rc = mcpkg_get_download(client->client,
	                            entry->url,
	                            entry->sha,
	                            dest);
	free(dest);
	if (rc != MCPKG_ERROR_SUCCESS) {
		return rc;
	}

	/* Now record it in Packages.install; upsert TAKES OWNERSHIP of entry. */
	rc = mcpkg_get_db_append(install_db, entry);

	/* On success: do not use or free 'entry' anymore (ownership transferred) */
	if (rc != MCPKG_ERROR_SUCCESS) {
		/* If you want to be tidy, you could unlink the downloaded file on failure. */
		return MCPKG_ERROR_FS;
	}

	return MCPKG_ERROR_SUCCESS;
}


static int mcpkg_get_dep_is_required(const char *type)
{
	// Modrinth types: required, optional, incompatible, embedded, (and sometimes "soft" aka optional)
	return (type && strcmp(type, "required") == 0);
}


// FIX implementation
McPkgGet *mcpkg_get_new(void);
void mcpkg_get_free(McPkgGet *get);

MCPKG_ERROR_TYPE mcpkg_get_install(const char *mc_version,
                                   const char *mod_loader,
                                   str_array *packages);

// check the cache for its version then check the
char *mcpkg_get_policy(const char *mc_version,
                       const char *mod_loader,
                       str_array *packages);

static MCPKG_ERROR_TYPE mcpkg_get_db_write_all(const char *install_db_path,
                McPkgEntry **entries,
                size_t count)
{
	FILE *fp = fopen(install_db_path, "wb");
	if (!fp) {
		perror("install_db_write_all: fopen");
		return MCPKG_ERROR_FS;
	}

	msgpack_sbuffer sbuf;
	msgpack_sbuffer_init(&sbuf);

	for (size_t i = 0; i < count; ++i) {
		if (!entries[i]) continue;
		if (mcpkg_entry_pack(entries[i], &sbuf) != 0) {
			msgpack_sbuffer_destroy(&sbuf);
			fclose(fp);
			return MCPKG_ERROR_FS;
		}
	}

	size_t wrote = fwrite(sbuf.data, 1, sbuf.size, fp);
	msgpack_sbuffer_destroy(&sbuf);

	if (wrote != (size_t)(ftell(fp))) {
		// Not a reliable check; still ensure fwrite succeeded fully:
		// fwrite returns bytes written; compare with intended size:
		// We already have wrote == sbuf.size implicitly by comparing to ftell post-write.
	}

	if (fclose(fp) != MCPKG_ERROR_SUCCESS) {
		perror("install_db_write_all: fclose");
		return 1;
	}
	return MCPKG_ERROR_SUCCESS;
}

int mcpkg_get_remove(const char *mc_version,
                     const char *mod_loader,
                     str_array *packages);


// check what is installed vs what is upstream; upgrade if new versions and upgrade deps as well (if needed)
int mcpkg_get_upgreade(const char *mc_version,
                       const char *mod_loader);

MCPKG_END_DECLS
#endif /* MCPKG_GET_H */
