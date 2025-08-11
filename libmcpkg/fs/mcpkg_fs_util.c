#include "mcpkg_fs_util.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#  include <windows.h>
#else
#include <limits.h>
#endif


int mcpkg_fs_is_separator(char c)
{
    return c == '/' || c == '\\';
}

static size_t safe_strlen(const char *s)
{
    return s ? strlen(s) : 0;
}

MCPKG_FS_ERROR mcpkg_fs_join2(const char *a, const char *b, char **out)
{
    size_t la, lb, need_sep, n;
    char *p;

    if (!a || !b || !out)
        return MCPKG_FS_ERR_NULL_PARAM;

    la = safe_strlen(a);
    lb = safe_strlen(b);
    need_sep = (la > 0 && !mcpkg_fs_is_separator(a[la - 1])) ? 1 : 0;

    if (la > SIZE_MAX - lb - need_sep - 1)
        return MCPKG_FS_ERR_OVERFLOW;

    n = la + need_sep + lb + 1;
    p = (char *)malloc(n);
    if (!p)
        return MCPKG_FS_ERR_OOM;

    memcpy(p, a, la);
    if (need_sep)
        p[la] = '/';
    memcpy(p + la + need_sep, b, lb);
    p[la + need_sep + lb] = '\0';

    *out = p;
    return MCPKG_FS_OK;
}

static MCPKG_FS_ERROR join5(const char *a, const char *b, const char *c,
                            const char *d, const char *e, char **out)
{
    MCPKG_FS_ERROR er;
    char *ab = NULL, *abc = NULL, *abcd = NULL, *abcde = NULL;

    er = mcpkg_fs_join2(a, b, &ab);
    if (er != MCPKG_FS_OK)
        return er;
    er = mcpkg_fs_join2(ab, c, &abc);
    free(ab);
    if (er != MCPKG_FS_OK)
        return er;
    er = mcpkg_fs_join2(abc, d, &abcd);
    free(abc);
    if (er != MCPKG_FS_OK)
        return er;
    er = mcpkg_fs_join2(abcd, e, &abcde);
    free(abcd);
    if (er != MCPKG_FS_OK)
        return er;

    *out = abcde;
    return MCPKG_FS_OK;
}

MCPKG_FS_ERROR
mcpkg_fs_path_mods_dir(char **out_path, const char *root,
                       const char *loader, const char *codename,
                       const char *version)
{
    if (!out_path || !root || !loader || !codename || !version)
        return MCPKG_FS_ERR_NULL_PARAM;

    return join5(root, loader, codename, version, "mods", out_path);
}

MCPKG_FS_ERROR
mcpkg_fs_path_db_file(char **out_path, const char *root,
                      const char *loader, const char *codename,
                      const char *version)
{
    MCPKG_FS_ERROR er;
    char *mods = NULL;

    if (!out_path || !root || !loader || !codename || !version)
        return MCPKG_FS_ERR_NULL_PARAM;

    er = join5(root, loader, codename, version, "mods", &mods);
    if (er != MCPKG_FS_OK)
        return er;

    er = mcpkg_fs_join2(mods, "Packages.install", out_path);
    free(mods);
    return er;
}

MCPKG_FS_ERROR mcpkg_fs_config_dir(char **out_dir)
{
    const char *home;
    char *tmp = NULL;

    if (!out_dir)
        return MCPKG_FS_ERR_NULL_PARAM;

#ifdef _WIN32
    home = getenv("APPDATA");
    if (!home || !*home)
        return MCPKG_FS_ERR_NOT_FOUND;
    if (mcpkg_fs_join2(home, "mcpkg", &tmp) != MCPKG_FS_OK)
        return MCPKG_FS_ERR_OOM;
#else
    home = getenv("HOME");
    if (!home || !*home)
        return MCPKG_FS_ERR_NOT_FOUND;
    if (mcpkg_fs_join2(home, ".config", &tmp) != MCPKG_FS_OK)
        return MCPKG_FS_ERR_OOM;
    {
        char *tmp2 = NULL;
        MCPKG_FS_ERROR er = mcpkg_fs_join2(tmp, "mcpkg", &tmp2);
        free(tmp);
        if (er != MCPKG_FS_OK)
            return er;
        tmp = tmp2;
    }
#endif
    *out_dir = tmp;
    return MCPKG_FS_OK;
}

MCPKG_FS_ERROR mcpkg_fs_config_file(char **out_file)
{
    char *dir = NULL, *out = NULL;
    MCPKG_FS_ERROR er;

    if (!out_file)
        return MCPKG_FS_ERR_NULL_PARAM;

    er = mcpkg_fs_config_dir(&dir);
    if (er != MCPKG_FS_OK)
        return er;

    er = mcpkg_fs_join2(dir, "config", &out);
    free(dir);
    if (er != MCPKG_FS_OK)
        return er;

    *out_file = out;
    return MCPKG_FS_OK;
}
