#include "mcpkg_fs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


#ifdef _WIN32
// ---------- Windows implementations I hope ..... ----------
MCPKG_ERROR_TYPE mcpkg_fs_cp_dir(const char *src, const char *dst)
{
	if (mkdir_p(dst) != MCPKG_ERROR_SUCCESS)
		return MCPKG_ERROR_FS;

	char *pattern = NULL;
	if (asprintf(&pattern, "%s\\*.*", src) < 0)
		return MCPKG_ERROR_OOM;

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(pattern, &ffd);
	free(pattern);
	if (hFind == INVALID_HANDLE_VALUE)
		return MCPKG_ERROR_FS;

	do {
		const char *n = ffd.cFileName;
		if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0)
			continue;

		char *s = mcpkg_fs_join(src, n);
		char *t = mcpkg_fs_join(dst, n);
		if (!s || !t) {
			free(s);
			free(t);
			FindClose(hFind);
			return MCPKG_ERROR_OOM;
		}

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (mcpkg_fs_cp_dir(s, t) != MCPKG_ERROR_SUCCESS) {
				free(s);
				free(t);
				FindClose(hFind);
				return MCPKG_ERROR_FS;
			}
		} else {
			if (mcpkg_fs_cp_file(s, t) != MCPKG_ERROR_SUCCESS) {
				free(s);
				free(t);
				FindClose(hFind);
				return MCPKG_ERROR_FS;
			}
		}
		free(s);
		free(t);
	} while (FindNextFileA(hFind, &ffd));
	FindClose(hFind);
	return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE mcpkg_fs_rm_dir(const char *path)
{
	// Recursively delete
	char *pattern = NULL;
	if (asprintf(&pattern, "%s\\*.*", path) < 0)
		return MCPKG_ERROR_OOM;

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(pattern, &ffd);
	free(pattern);
	if (hFind == INVALID_HANDLE_VALUE) {
		// maybe not a dir; try DeleteFileA then RemoveDirectoryA
		if (DeleteFileA(path))
			return MCPKG_ERROR_SUCCESS;
		if (RemoveDirectoryA(path))
			return MCPKG_ERROR_SUCCESS;
		return MCPKG_ERROR_FS;
	}

	do {
		const char *n = ffd.cFileName;
		if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0)
			continue;

		char *p = mcpkg_fs_join(path, n);
		if (!p) {
			FindClose(hFind);
			return MCPKG_ERROR_OOM;
		}

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			mcpkg_fs_rm_dir(p);
			RemoveDirectoryA(p);
		} else {
			DeleteFileA(p);
		}
		free(p);
	} while (FindNextFileA(hFind, &ffd));
	FindClose(hFind);

	return RemoveDirectoryA(path) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

MCPKG_ERROR_TYPE mcpkg_fs_unlink(const char *path)
{
	if (DeleteFileA(path))
		return MCPKG_ERROR_SUCCESS;
	if (RemoveDirectoryA(path))
		return MCPKG_ERROR_SUCCESS;
	return MCPKG_ERROR_FS;
}
#else


MCPKG_ERROR_TYPE mcpkg_fs_cp_dir(const char *src, const char *dst)
{
	if (mcpkg_fs_mkdir(dst) != MCPKG_ERROR_SUCCESS)
		return MCPKG_ERROR_FS;

	DIR *d = opendir(src);
	if (!d)
		return MCPKG_ERROR_FS;

	struct dirent *ent;
	while ((ent = readdir(d))) {
		const char *n = ent->d_name;
		if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0)
			continue;

		char *s = mcpkg_fs_join(src, n);
		char *t = mcpkg_fs_join(dst, n);
		if (!s || !t) {
			free(s);
			free(t);
			closedir(d);
			return MCPKG_ERROR_OOM;
		}

		struct stat st;
		if (lstat(s, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				if (mcpkg_fs_cp_dir(s, t) != MCPKG_ERROR_SUCCESS) {
					free(s);
					free(t);
					closedir(d);
					return MCPKG_ERROR_FS;
				}
			} else if (S_ISREG(st.st_mode)) {
				if (mcpkg_fs_cp_file(s, t) != MCPKG_ERROR_SUCCESS) {
					free(s);
					free(t);
					closedir(d);
					return MCPKG_ERROR_FS;
				}
			} // ignore others
		}
		free(s);
		free(t);
	}
	closedir(d);
	return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE mcpkg_fs_rm_r(const char *path)
{
	struct stat st;
	if (lstat(path, &st) != 0)
		return MCPKG_ERROR_FS;

	if (S_ISDIR(st.st_mode))
		return mcpkg_fs_rm_dir(path);

	return (unlink(path) == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

MCPKG_ERROR_TYPE mcpkg_fs_unlink(const char *path)
{
	struct stat st;
	if (lstat(path, &st) != 0)
		return MCPKG_ERROR_FS;

	if (S_ISDIR(st.st_mode))
		return (rmdir(path) == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;

	return (unlink(path) == 0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

#endif

MCPKG_ERROR_TYPE mcpkg_fs_mkdir(const char *path)
{
	if (!path || !*path)
		return MCPKG_ERROR_FS;

	char *tmp = strdup(path);
	if (!tmp)
		return MCPKG_ERROR_GENERAL;

	// Strip trailing slashes (but keep root "/")
	size_t len = strlen(tmp);
	while (len > 1 && tmp[len - 1] == '/')
		tmp[--len] = '\0';

	// Walk components
	for (char *p = tmp + 1; *p; ++p) {
		if (*p == '/') {
			*p = '\0';
			if (mcpkg_fs_dir_exists(tmp) != MCPKG_ERROR_SUCCESS) {
				if (mkdir(tmp, NEW_DIR_PREM) != 0 && errno != EEXIST) {
					free(tmp);
					return MCPKG_ERROR_FS;
				}
			}
			*p = '/';
		}
	}
	if (mcpkg_fs_dir_exists(tmp) != MCPKG_ERROR_SUCCESS) {
		if (mkdir(tmp, NEW_DIR_PREM) != 0 && errno != EEXIST) {
			free(tmp);
			return MCPKG_ERROR_FS;
		}
	}

	free(tmp);
	return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE mcpkg_fs_touch(const char *path)
{
	if (!path)
		return MCPKG_ERROR_FS;

	int fd = open(path, O_WRONLY | O_CREAT, 0644);
	if (fd < 0)
		return MCPKG_ERROR_FS;

// Update times to "now" lol
#if defined(HAVE_FUTIMENS) || (_POSIX_C_SOURCE >= 200809L)
	struct timespec ts[2];
	ts[0].tv_sec = ts[1].tv_sec = time(NULL);
	ts[0].tv_nsec = ts[1].tv_nsec = 0;
	if (futimens(fd, ts) != 0) {
		// not fatal if FS doesn’t support it
	}
#endif
	close(fd);
	return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE mcpkg_fs_ln_sf(const char *target,
                                const char *link_path,
                                int overwrite)
{
	if (!target || !link_path)
		return MCPKG_ERROR_FS;

	if (!overwrite) {
		if (symlink(target, link_path) == 0)
			return MCPKG_ERROR_SUCCESS;
		return (errno == EEXIST) ? MCPKG_ERROR_FS : MCPKG_ERROR_FS;
	}

	// If exists, remove it (file/dir/link)
	struct stat st;
	if (lstat(link_path, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			// refuse to clobber directories
			return MCPKG_ERROR_FS;
		}
		if (unlink(link_path) != 0)
			return MCPKG_ERROR_FS;
	}

	if (symlink(target, link_path) != 0)
		return MCPKG_ERROR_FS;
	return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE mcpkg_fs_read_cache(const char *path,
                                     char **buffer,
                                     size_t *size)
{
	if (!path || !buffer || !size)
		return MCPKG_ERROR_OOM;

	FILE *fp = fopen(path, "rb");
	if (!fp)
		return MCPKG_ERROR_FS;

	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return MCPKG_ERROR_FS;
	}
	long end = ftell(fp);
	if (end < 0) {
		fclose(fp);
		return MCPKG_ERROR_FS;
	}
	if (fseek(fp, 0, SEEK_SET) != 0) {
		fclose(fp);
		return MCPKG_ERROR_FS;
	}

	*size = (size_t)end;
	*buffer = (char *)malloc(*size + 1); //MEMORY +1
	if (!*buffer) {
		fclose(fp);
		return MCPKG_ERROR_GENERAL;
	}

	size_t rd = fread(*buffer, 1, *size, fp);
	fclose(fp);
	if (rd != *size) {
		free(*buffer);
		*buffer = NULL;
		*size = 0;
		return MCPKG_ERROR_FS;
	}
	(*buffer)[*size] = '\0';
	return MCPKG_ERROR_SUCCESS;
}

