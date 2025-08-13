#include "fs/mcpkg_fs_std_paths.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

/* ----- helpers ----- */

static inline char *dup_cstr(const char *s)
{
	size_t n;
	char *p;

	if (!s)
		return NULL;

	n = strlen(s);
	p = (char *)malloc(n + 1);
	if (!p)
		return NULL;

	memcpy(p, s, n + 1);
	return p;
}

#ifndef _WIN32
static inline const char *linux_path_for(MCPKG_FS_LOCATION loc)
{
	switch (loc) {
		case MCPKG_FS_TMP:
			return "/tmp";
		case MCPKG_FS_HOME: {
			const char *h = getenv("HOME");
			return (h && *h) ? h : NULL;
		}
		case MCPKG_FS_CONFIG:
			return "/etc";
		case MCPKG_FS_SHARE:
			return "/usr/share";
		case MCPKG_FS_CACHE:
			return "/var/cache";
		default:
			return NULL;
	}
}
#else
/* Fill 'buf' with a Windows path for 'loc'. Returns 0 on success. */
static int windows_path_for(MCPKG_FS_LOCATION loc, char *buf, size_t bufsz)
{
	DWORD n;

	if (!buf || bufsz == 0)
		return -1;

	buf[0] = '\0';

	switch (loc) {
		case MCPKG_FS_TMP:
			n = GetTempPathA((DWORD)bufsz, buf);
			if (n == 0 || n >= bufsz)
				return -1;
			return 0;

		case MCPKG_FS_HOME: {
			const char *p = getenv("USERPROFILE");
			if (!p || !*p) {
				const char *drv = getenv("HOMEDRIVE");
				const char *pth = getenv("HOMEPATH");
				if (drv && pth && *drv && *pth) {
					/* Construct HOMEDRIVE + HOMEPATH */
					size_t need = strlen(drv) + strlen(pth) + 1;
					if (need >= bufsz)
						return -1;
					strcpy(buf, drv);
					strcat(buf, pth);
					return 0;
				}
				return -1;
			}
			if (strlen(p) >= bufsz)
				return -1;
			strcpy(buf, p);
			return 0;
		}

		case MCPKG_FS_CONFIG: {
			/* Machine-wide config equivalent of /etc */
			const char *p = getenv("PROGRAMDATA"); /* e.g., C:\ProgramData */
			if (!p || !*p)
				return -1;
			if (strlen(p) >= bufsz)
				return -1;
			strcpy(buf, p);
			return 0;
		}

		case MCPKG_FS_SHARE: {
			/* Shared data; use ProgramData (common to all users) */
			const char *p = getenv("PROGRAMDATA");
			if (!p || !*p)
				return -1;
			if (strlen(p) >= bufsz)
				return -1;
			strcpy(buf, p);
			return 0;
		}

		case MCPKG_FS_CACHE: {
			/* Per-user cache/data: prefer LOCALAPPDATA, fallback APPDATA */
			const char *p = getenv("LOCALAPPDATA");
			if ((!p || !*p))
				p = getenv("APPDATA");
			if (!p || !*p)
				return -1;
			if (strlen(p) >= bufsz)
				return -1;
			strcpy(buf, p);
			return 0;
		}

		default:
			return -1;
	}
}
#endif

/* ----- API ----- */

MCPKG_FS_ERROR
mcpkg_fs_default_dir(MCPKG_FS_LOCATION location, McPkgStringList *out)
{
#ifndef _WIN32
	const char *p;
	MCPKG_CONTAINER_ERROR cer;

	if (!out)
		return MCPKG_FS_ERR_NULL_PARAM;

	p = linux_path_for(location);
	if (!p)
		return MCPKG_FS_ERR_NOT_FOUND;

	cer = mcpkg_stringlist_push(out, p);
	if (cer != MCPKG_CONTAINER_OK)
		return MCPKG_FS_ERR_OOM;

	return MCPKG_FS_OK;
#else
	char buf[4096];
	MCPKG_CONTAINER_ERROR cer;

	if (!out)
		return MCPKG_FS_ERR_NULL_PARAM;

	if (windows_path_for(location, buf, sizeof buf) != 0)
		return MCPKG_FS_ERR_NOT_FOUND;

	/* mcpkg_stringlist_push will strdup() the buffer contents */
	cer = mcpkg_stringlist_push(out, buf);
	if (cer != MCPKG_CONTAINER_OK)
		return MCPKG_FS_ERR_OOM;

	return MCPKG_FS_OK;
#endif
}

MCPKG_FS_ERROR
mcpkg_fs_writable_dir(MCPKG_FS_LOCATION location, char **out)
{
#ifndef _WIN32
	const char *p;

	if (!out)
		return MCPKG_FS_ERR_NULL_PARAM;

	*out = NULL;

	p = linux_path_for(location);
	if (!p)
		return MCPKG_FS_ERR_NOT_FOUND;

	*out = dup_cstr(p);
	if (!*out)
		return MCPKG_FS_ERR_OOM;

	return MCPKG_FS_OK;
#else
	char buf[4096];

	if (!out)
		return MCPKG_FS_ERR_NULL_PARAM;

	*out = NULL;

	if (windows_path_for(location, buf, sizeof buf) != 0)
		return MCPKG_FS_ERR_NOT_FOUND;

	*out = dup_cstr(buf);
	if (!*out)
		return MCPKG_FS_ERR_OOM;

	return MCPKG_FS_OK;
#endif
}
