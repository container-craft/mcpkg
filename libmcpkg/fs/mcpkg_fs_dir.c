#include "fs/mcpkg_fs_dir.h"
#include "fs/mcpkg_fs_util.h"
#include "fs/mcpkg_fs_file.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <direct.h>
#else
#  include <dirent.h>
#  include <unistd.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#endif

int mcpkg_fs_dir_exists(const char *path)
{
#ifdef _WIN32
	DWORD attr;

	if (!path)
		return -1;

	attr = GetFileAttributesA(path);
	if (attr == INVALID_FILE_ATTRIBUTES)
		return 0;
	return (attr & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
#else
	struct stat st;

	if (!path)
		return -1;

	if (stat(path, &st) != 0)
		return 0;
	return S_ISDIR(st.st_mode) ? 1 : 0;
#endif
}

static int is_sep(char c)
{
	return c == '/' || c == '\\';
}

MCPKG_FS_ERROR mcpkg_fs_mkdir_p(const char *path)
{
	char *tmp, *p;

	if (!path || !*path)
		return MCPKG_FS_ERR_NULL_PARAM;

	tmp = strdup(path);
	if (!tmp)
		return MCPKG_FS_ERR_OOM;

	/* strip trailing separators, but keep root like "C:\" or "/" */
	{
		size_t len = strlen(tmp);
		while (len > 1 && is_sep(tmp[len - 1]))
			tmp[--len] = '\0';
	}

	/* walk components */
	for (p = tmp + 1; *p; ++p) {
		if (is_sep(*p)) {
			*p = '\0';
#ifdef _WIN32
			if (mcpkg_fs_dir_exists(tmp) <= 0) {
				if (_mkdir(tmp) != 0 && errno != EEXIST) {
					free(tmp);
					return MCPKG_FS_ERR_IO;
				}
			}
#else
			if (mcpkg_fs_dir_exists(tmp) <= 0) {
				if (mkdir(tmp, MCPKG_FS_DIR_PERM) != 0 &&
				    errno != EEXIST) {
					free(tmp);
					return MCPKG_FS_ERR_IO;
				}
			}
#endif
			*p = '/';
		}
	}

#ifdef _WIN32
	if (mcpkg_fs_dir_exists(tmp) <= 0) {
		if (_mkdir(tmp) != 0 && errno != EEXIST) {
			free(tmp);
			return MCPKG_FS_ERR_IO;
		}
	}
#else
	if (mcpkg_fs_dir_exists(tmp) <= 0) {
		if (mkdir(tmp, MCPKG_FS_DIR_PERM) != 0 && errno != EEXIST) {
			free(tmp);
			return MCPKG_FS_ERR_IO;
		}
	}
#endif

	free(tmp);
	return MCPKG_FS_OK;
}

MCPKG_FS_ERROR mcpkg_fs_cp_dir(const char *src, const char *dst, int overwrite)
{
#ifdef _WIN32
	HANDLE hFind;
	WIN32_FIND_DATAA ffd;
	char pattern[MAX_PATH];
	int ok;

	if (!src || !dst)
		return MCPKG_FS_ERR_NULL_PARAM;

	/* ensure dst exists */
	if (mcpkg_fs_mkdir_p(dst) != MCPKG_FS_OK)
		return MCPKG_FS_ERR_IO;

	ok = snprintf(pattern, sizeof(pattern), "%s\\*.*", src);
	if (ok <= 0 || (size_t)ok >= sizeof(pattern))
		return MCPKG_FS_ERR_OVERFLOW;

	hFind = FindFirstFileA(pattern, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
		return MCPKG_FS_ERR_IO;

	do {
		const char *n = ffd.cFileName;
		char *s = NULL, *t = NULL;
		MCPKG_FS_ERROR er;

		if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0)
			continue;

		er = mcpkg_fs_join2(src, n, &s);
		if (er != MCPKG_FS_OK) {
			FindClose(hFind);
			return er;
		}
		er = mcpkg_fs_join2(dst, n, &t);
		if (er != MCPKG_FS_OK) {
			free(s);
			FindClose(hFind);
			return er;
		}

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			er = mcpkg_fs_cp_dir(s, t, overwrite);
		} else {
			er = mcpkg_fs_cp_file(s, t, overwrite);
		}
		free(s);
		free(t);
		if (er != MCPKG_FS_OK) {
			FindClose(hFind);
			return er;
		}
	} while (FindNextFileA(hFind, &ffd));
	FindClose(hFind);
	return MCPKG_FS_OK;
#else
	DIR *d;
	struct dirent *ent;

	if (!src || !dst)
		return MCPKG_FS_ERR_NULL_PARAM;

	if (mcpkg_fs_mkdir_p(dst) != MCPKG_FS_OK)
		return MCPKG_FS_ERR_IO;

	d = opendir(src);
	if (!d)
		return MCPKG_FS_ERR_IO;

	while ((ent = readdir(d))) {
		struct stat st;
		char *s = NULL, *t = NULL;
		MCPKG_FS_ERROR er;

		if (strcmp(ent->d_name, ".") == 0 ||
		    strcmp(ent->d_name, "..") == 0)
			continue;

		er = mcpkg_fs_join2(src, ent->d_name, &s);
		if (er != MCPKG_FS_OK) {
			closedir(d);
			return er;
		}
		er = mcpkg_fs_join2(dst, ent->d_name, &t);
		if (er != MCPKG_FS_OK) {
			free(s);
			closedir(d);
			return er;
		}

		if (lstat(s, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				er = mcpkg_fs_cp_dir(s, t, overwrite);
			} else if (S_ISREG(st.st_mode)) {
				er = mcpkg_fs_cp_file(s, t, overwrite);
			} else {
				er = MCPKG_FS_OK; /* ignore others */
			}
			free(s);
			free(t);
			if (er != MCPKG_FS_OK) {
				closedir(d);
				return er;
			}
		} else {
			free(s);
			free(t);
		}
	}
	closedir(d);
	return MCPKG_FS_OK;
#endif
}

MCPKG_FS_ERROR mcpkg_fs_rm_r(const char *path)
{
#ifdef _WIN32
	char pattern[MAX_PATH];
	WIN32_FIND_DATAA ffd;
	HANDLE hFind;
	int ok;

	if (!path)
		return MCPKG_FS_ERR_NULL_PARAM;

	ok = snprintf(pattern, sizeof(pattern), "%s\\*.*", path);
	if (ok <= 0 || (size_t)ok >= sizeof(pattern))
		return MCPKG_FS_ERR_OVERFLOW;

	hFind = FindFirstFileA(pattern, &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		/* try file then directory */
		if (DeleteFileA(path))
			return MCPKG_FS_OK;
		return RemoveDirectoryA(path) ? MCPKG_FS_OK : MCPKG_FS_ERR_IO;
	}

	do {
		const char *n = ffd.cFileName;
		char *p = NULL;
		MCPKG_FS_ERROR er;

		if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0)
			continue;

		er = mcpkg_fs_join2(path, n, &p);
		if (er != MCPKG_FS_OK) {
			FindClose(hFind);
			return er;
		}

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			er = mcpkg_fs_rm_r(p);
			free(p);
			if (er != MCPKG_FS_OK) {
				FindClose(hFind);
				return er;
			}
		} else {
			if (!DeleteFileA(p)) {
				free(p);
				FindClose(hFind);
				return MCPKG_FS_ERR_IO;
			}
			free(p);
		}
	} while (FindNextFileA(hFind, &ffd));
	FindClose(hFind);

	return RemoveDirectoryA(path) ? MCPKG_FS_OK : MCPKG_FS_ERR_IO;
#else
	struct stat st;

	if (!path)
		return MCPKG_FS_ERR_NULL_PARAM;

	if (lstat(path, &st) != 0)
		return MCPKG_FS_ERR_IO;

	if (!S_ISDIR(st.st_mode)) {
		return (unlink(path) == 0) ? MCPKG_FS_OK : MCPKG_FS_ERR_IO;
	}

	/* directory: recurse */
	{
		DIR *d = opendir(path);
		struct dirent *ent;

		if (!d)
			return MCPKG_FS_ERR_IO;

		while ((ent = readdir(d))) {
			char *p = NULL;
			MCPKG_FS_ERROR er;

			if (strcmp(ent->d_name, ".") == 0 ||
			    strcmp(ent->d_name, "..") == 0)
				continue;

			er = mcpkg_fs_join2(path, ent->d_name, &p);
			if (er != MCPKG_FS_OK) {
				closedir(d);
				return er;
			}
			er = mcpkg_fs_rm_r(p);
			free(p);
			if (er != MCPKG_FS_OK) {
				closedir(d);
				return er;
			}
		}
		closedir(d);
	}
	return (rmdir(path) == 0) ? MCPKG_FS_OK : MCPKG_FS_ERR_IO;
#endif
}
