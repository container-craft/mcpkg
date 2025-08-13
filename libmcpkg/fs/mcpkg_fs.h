#ifndef MCPKG_FS_H
#define MCPKG_FS_H

#include <stddef.h>
#include <sys/stat.h>
#include <zstd.h>



#if _WIN32
#include "utils/compat.h"
#include <windows.h>
#include <io.h>
#include <direct.h>
#else
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif


#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

#ifndef NEW_DIR_PREM
#define NEW_DIR_PREM 0755
#endif

#ifndef NEW_FILE_PREM
#define NEW_FILE_PREM 0644
#endif
static int mcpkg_fs_is_separator(char c)
{
	return c == '/';
}
static char *mcpkg_fs_join(const char *a,
                           const char *b)
{
	char *out = NULL;
	size_t la = strlen(a);
	size_t lb = strlen(b);

	int need_sep = (la > 0 && !mcpkg_fs_is_separator(a[la - 1]));
	if (asprintf(&out, "%s%s%s", a, need_sep ? "/" : "", b) < 0)
		return NULL;

	return out;
}


#ifdef _WIN32
static MCPKG_ERROR_TYPE mcpkg_fs_cp_file(const char *src, const char *dst)
{
	// Ensure parent dir exists
	char *dup = _strdup(dst);
	if (!dup) return MCPKG_ERROR_OOM;
	char *slash = strrchr(dup, '\\');
	if (!slash)
		slash = strrchr(dup, '/');
	if (slash) {
		*slash = 0;
		_mkdir(dup);
	} // best-effort
	free(dup);

	if (!CopyFileA(src, dst, FALSE))
		return MCPKG_ERROR_FS;
	return MCPKG_ERROR_SUCCESS;
}
#else
static MCPKG_ERROR_TYPE mcpkg_fs_cp_file(const char *src,
                const char *dst)
{
	int in = open(src, O_RDONLY);
	if (in < 0)
		return MCPKG_ERROR_FS;
	int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, NEW_FILE_PREM);
	if (out < 0) {
		close(in);
		return MCPKG_ERROR_FS;
	}

	char buf[64 * 1024];
	ssize_t n;
	while ((n = read(in, buf, sizeof buf)) > 0) {
		ssize_t w = write(out, buf, n);
		if (w != n) {
			close(in);
			close(out);
			return MCPKG_ERROR_FS;
		}
	}
	close(in);
	close(out);
	return (n < 0) ? MCPKG_ERROR_FS : MCPKG_ERROR_SUCCESS;
}
#endif



// /var/cache/..../
static MCPKG_ERROR_TYPE mcpkg_fs_mods_dir(char **out_path,
                const char *root,
                const char *loader,
                const char *codename,
                const char *version)
{
	if (asprintf(out_path, "%s/%s/%s/%s/mods", root, loader, codename, version) < 0)
		return MCPKG_ERROR_FS;
	return MCPKG_ERROR_SUCCESS;
}

// /var/cache/..../mods
static MCPKG_ERROR_TYPE mcpkg_fs_db_dir(char **out_path,
                                        const char *root,
                                        const char *loader,
                                        const char *codename,
                                        const char *version)
{
	if (asprintf(out_path,
	             "%s/%s/%s/%s/mods/Packages.install",
	             root,
	             loader,
	             codename,
	             version) < 0)
		return MCPKG_ERROR_FS;
	return MCPKG_ERROR_SUCCESS;
}


static char *mcpkg_fs_config_dir(void)
{
	const char *home = getenv("HOME");
#ifdef _WIN32
	home = getenv("APPDATA");
#else
	char *out = NULL;
	if (home && *home) {
		if (asprintf(&out, "%s/.config/mcpkg", home) < 0)
			return NULL;
		return out;
	}
	return out;
#endif
}

static char *mcpkg_fs_config_file(void)
{
	char *dir = mcpkg_fs_config_dir();
	if (!dir)
		return NULL;

	char *out = NULL;
	if (asprintf(&out, "%s/config", dir) < 0) {
		free(dir);
		return NULL;
	}
	free(dir);
	return out;
}


static MCPKG_ERROR_TYPE mcpkg_fs_compressed_file(const char *file_path,
                const void *data,
                size_t size)
{
	FILE *fp = fopen(file_path, "wb");
	if (!fp) {
		perror("Failed to open compressed file");
		return MCPKG_ERROR_GENERAL;
	}

	size_t compressed_size = ZSTD_compressBound(size);
	void *compressed_data = malloc(compressed_size);
	if (!compressed_data) {
		fclose(fp);
		return MCPKG_ERROR_GENERAL;
	}

	size_t result = ZSTD_compress(compressed_data, compressed_size, data, size, 1);
	if (ZSTD_isError(result)) {
		fprintf(stderr, "ZSTD compression failed: %s\n", ZSTD_getErrorName(result));
		free(compressed_data);
		fclose(fp);
		return MCPKG_ERROR_GENERAL;
	}

	fwrite(compressed_data, 1, result, fp);
	free(compressed_data);
	fclose(fp);
	return MCPKG_ERROR_SUCCESS;
}

static MCPKG_ERROR_TYPE mcpkg_fs_dir_exists(const char *path)
{
	struct stat path_stat;
	if (stat(path, &path_stat) != 0)
		return MCPKG_ERROR_FS;
	return S_ISDIR(path_stat.st_mode) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}


static MCPKG_ERROR_TYPE mcpkg_fs_rm_dir(const char *path)
{
	DIR *d = opendir(path);
	if (!d)
		return MCPKG_ERROR_FS;

	struct dirent *ent;
	while ((ent = readdir(d))) {
		const char *n = ent->d_name;
		if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0)
			continue;
		char *p = mcpkg_fs_join(path, n);
		if (!p) {
			closedir(d);
			return MCPKG_ERROR_OOM;
		}
		struct stat st;
		if (lstat(p, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				mcpkg_fs_rm_dir(p);
				rmdir(p);
			} else {
				unlink(p);
			}
		}
		free(p);
	}
	closedir(d);
	return (rmdir(path) == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

MCPKG_ERROR_TYPE mcpkg_fs_mkdir(const char *path);


MCPKG_ERROR_TYPE mcpkg_fs_touch(const char *path);


MCPKG_ERROR_TYPE mcpkg_fs_ln_sf(const char *target,
                                const char *link_path,
                                int overwrite);

MCPKG_ERROR_TYPE mcpkg_fs_read_cache(const char *path,
                                     char **buffer,
                                     size_t *size);
MCPKG_ERROR_TYPE mcpkg_fs_cp_dir(const char *src, const char *dst);
MCPKG_ERROR_TYPE mcpkg_fs_rm_r(const char *path);
MCPKG_ERROR_TYPE mcpkg_fs_unlink(const char *path);

MCPKG_END_DECLS
#endif // MCPKG_FS_H
