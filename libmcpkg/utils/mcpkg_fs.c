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

#ifndef DIR_MODE
#define DIR_MODE 0755
#endif
// utils/mcpkg_fs.c (additions)

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#define PATH_SEP '\\'
static int path_is_sep(char c){ return c=='\\' || c=='/'; }
#else
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#define PATH_SEP '/'
static int path_is_sep(char c){ return c=='/'; }
#endif

static char *join2(const char *a, const char *b) {
    char *out=NULL;
    size_t la=strlen(a), lb=strlen(b);
    int need_sep = (la>0 && !path_is_sep(a[la-1]));
#ifdef _WIN32
    if (asprintf(&out, "%s%s%s", a, need_sep?"/":"", b) < 0) return NULL;
#else
    if (asprintf(&out, "%s%s%s", a, need_sep?"/":"", b) < 0) return NULL;
#endif
    return out;
}

#ifndef _WIN32
// ---------- POSIX implementations ----------
static mcpkg_error_types copy_file_posix(const char *src, const char *dst) {
    int in = open(src, O_RDONLY);
    if (in < 0) return MCPKG_ERROR_FS;
    int out = open(dst, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (out < 0) { close(in); return MCPKG_ERROR_FS; }

    char buf[64*1024];
    ssize_t n;
    while ((n = read(in, buf, sizeof buf)) > 0) {
        ssize_t w = write(out, buf, n);
        if (w != n) { close(in); close(out); return MCPKG_ERROR_FS; }
    }
    close(in); close(out);
    return (n<0) ? MCPKG_ERROR_FS : MCPKG_ERROR_SUCCESS;
}

mcpkg_error_types mcpkg_copy_tree(const char *src, const char *dst) {
    if (mkdir_p(dst) != MCPKG_ERROR_SUCCESS) return MCPKG_ERROR_FS;

    DIR *d = opendir(src);
    if (!d) return MCPKG_ERROR_FS;

    struct dirent *ent;
    while ((ent = readdir(d))) {
        const char *n = ent->d_name;
        if (strcmp(n, ".")==0 || strcmp(n, "..")==0) continue;

        char *s = join2(src, n);
        char *t = join2(dst, n);
        if (!s || !t) { free(s); free(t); closedir(d); return MCPKG_ERROR_OOM; }

        struct stat st;
        if (lstat(s, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (mcpkg_copy_tree(s, t) != MCPKG_ERROR_SUCCESS) { free(s); free(t); closedir(d); return MCPKG_ERROR_FS; }
            } else if (S_ISREG(st.st_mode)) {
                if (copy_file_posix(s, t) != MCPKG_ERROR_SUCCESS) { free(s); free(t); closedir(d); return MCPKG_ERROR_FS; }
            } // ignore others
        }
        free(s); free(t);
    }
    closedir(d);
    return MCPKG_ERROR_SUCCESS;
}

static mcpkg_error_types remove_tree_posix(const char *path) {
    DIR *d = opendir(path);
    if (!d) return MCPKG_ERROR_FS;
    struct dirent *ent;
    while ((ent = readdir(d))) {
        const char *n = ent->d_name;
        if (strcmp(n, ".")==0 || strcmp(n, "..")==0) continue;
        char *p = join2(path, n);
        if (!p) { closedir(d); return MCPKG_ERROR_OOM; }
        struct stat st;
        if (lstat(p, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                remove_tree_posix(p);
                rmdir(p);
            } else {
                unlink(p);
            }
        }
        free(p);
    }
    closedir(d);
    return (rmdir(path)==0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

mcpkg_error_types mcpkg_remove_tree(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return MCPKG_ERROR_FS;
    if (S_ISDIR(st.st_mode)) return remove_tree_posix(path);
    return (unlink(path)==0) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

mcpkg_error_types mcpkg_unlink_any(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return MCPKG_ERROR_FS;
    if (S_ISDIR(st.st_mode)) return (rmdir(path)==0)?MCPKG_ERROR_SUCCESS:MCPKG_ERROR_FS;
    return (unlink(path)==0)?MCPKG_ERROR_SUCCESS:MCPKG_ERROR_FS;
}

#else
// ---------- Windows implementations I hope ..... ----------
static mcpkg_error_types copy_file_win(const char *src, const char *dst) {
    // Ensure parent dir exists
    char *dup = _strdup(dst);
    if (!dup) return MCPKG_ERROR_OOM;
    char *slash = strrchr(dup, '\\'); if (!slash) slash = strrchr(dup, '/');
    if (slash) { *slash = 0; _mkdir(dup); } // best-effort
    free(dup);

    if (!CopyFileA(src, dst, FALSE)) return MCPKG_ERROR_FS;
    return MCPKG_ERROR_SUCCESS;
}

mcpkg_error_types mcpkg_copy_tree(const char *src, const char *dst) {
    // Ensure dst exists
    if (mkdir_p(dst) != MCPKG_ERROR_SUCCESS) return MCPKG_ERROR_FS;

    char *pattern = NULL;
    if (asprintf(&pattern, "%s\\*.*", src) < 0) return MCPKG_ERROR_OOM;

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(pattern, &ffd);
    free(pattern);
    if (hFind == INVALID_HANDLE_VALUE) return MCPKG_ERROR_FS;

    do {
        const char *n = ffd.cFileName;
        if (strcmp(n, ".")==0 || strcmp(n, "..")==0) continue;

        char *s = join2(src, n);
        char *t = join2(dst, n);
        if (!s || !t) { free(s); free(t); FindClose(hFind); return MCPKG_ERROR_OOM; }

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (mcpkg_copy_tree(s, t) != MCPKG_ERROR_SUCCESS) { free(s); free(t); FindClose(hFind); return MCPKG_ERROR_FS; }
        } else {
            if (copy_file_win(s, t) != MCPKG_ERROR_SUCCESS) { free(s); free(t); FindClose(hFind); return MCPKG_ERROR_FS; }
        }
        free(s); free(t);
    } while (FindNextFileA(hFind, &ffd));
    FindClose(hFind);
    return MCPKG_ERROR_SUCCESS;
}

mcpkg_error_types mcpkg_remove_tree(const char *path) {
    // Recursively delete
    char *pattern = NULL;
    if (asprintf(&pattern, "%s\\*.*", path) < 0) return MCPKG_ERROR_OOM;

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(pattern, &ffd);
    free(pattern);
    if (hFind == INVALID_HANDLE_VALUE) {
        // maybe not a dir; try DeleteFileA then RemoveDirectoryA
        if (DeleteFileA(path)) return MCPKG_ERROR_SUCCESS;
        if (RemoveDirectoryA(path)) return MCPKG_ERROR_SUCCESS;
        return MCPKG_ERROR_FS;
    }

    do {
        const char *n = ffd.cFileName;
        if (strcmp(n, ".")==0 || strcmp(n, "..")==0) continue;

        char *p = join2(path, n);
        if (!p) { FindClose(hFind); return MCPKG_ERROR_OOM; }

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            mcpkg_remove_tree(p);
            RemoveDirectoryA(p);
        } else {
            DeleteFileA(p);
        }
        free(p);
    } while (FindNextFileA(hFind, &ffd));
    FindClose(hFind);

    return RemoveDirectoryA(path) ? MCPKG_ERROR_SUCCESS : MCPKG_ERROR_FS;
}

mcpkg_error_types mcpkg_unlink_any(const char *path) {
    if (DeleteFileA(path)) return MCPKG_ERROR_SUCCESS;
    if (RemoveDirectoryA(path)) return MCPKG_ERROR_SUCCESS;
    return MCPKG_ERROR_FS;
}
#endif


static int is_dir(const char *p) {
    struct stat st;
    if (stat(p, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

mcpkg_error_types mkdir_p(const char *path) {
    if (!path || !*path) return MCPKG_ERROR_FS;

    // Duplicate so we can mutate
    char *tmp = strdup(path);
    if (!tmp) return MCPKG_ERROR_GENERAL;

    // Strip trailing slashes (but keep root "/")
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') tmp[--len] = '\0';

    // Walk components
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (!is_dir(tmp)) {
                if (mkdir(tmp, DIR_MODE) != 0 && errno != EEXIST) {
                    free(tmp);
                    return MCPKG_ERROR_FS;
                }
            }
            *p = '/';
        }
    }
    if (!is_dir(tmp)) {
        if (mkdir(tmp, DIR_MODE) != 0 && errno != EEXIST) {
            free(tmp);
            return MCPKG_ERROR_FS;
        }
    }

    free(tmp);
    return MCPKG_ERROR_SUCCESS;
}

mcpkg_error_types touch(const char *path) {
    if (!path) return MCPKG_ERROR_FS;

    int fd = open(path, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) return MCPKG_ERROR_FS;

// Update times to "now"
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

mcpkg_error_types create_symlink_overwrite(const char *target, const char *link_path, int overwrite) {
    if (!target || !link_path) return MCPKG_ERROR_FS;

    if (!overwrite) {
        if (symlink(target, link_path) == 0) return MCPKG_ERROR_SUCCESS;
        return (errno == EEXIST) ? MCPKG_ERROR_FS : MCPKG_ERROR_FS;
    }

    // If exists, remove it (file/dir/link)
    struct stat st;
    if (lstat(link_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            // refuse to clobber directories
            return MCPKG_ERROR_FS;
        }
        if (unlink(link_path) != 0) return MCPKG_ERROR_FS;
    }

    if (symlink(target, link_path) != 0) return MCPKG_ERROR_FS;
    return MCPKG_ERROR_SUCCESS;
}

mcpkg_error_types create_symlink(const char *target, const char *link_path) {
    return create_symlink_overwrite(target, link_path, /*overwrite=*/0);
}

mcpkg_error_types mcpkg_read_cache_file(const char *path, char **buffer, size_t *size) {
    if (!path || !buffer || !size) return MCPKG_ERROR_PARSE;

    FILE *fp = fopen(path, "rb");
    if (!fp) return MCPKG_ERROR_FS;

    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return MCPKG_ERROR_FS; }
    long end = ftell(fp);
    if (end < 0) { fclose(fp); return MCPKG_ERROR_FS; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return MCPKG_ERROR_FS; }

    *size = (size_t)end;
    *buffer = (char *)malloc(*size + 1); // +1 so caller can treat as C-string if desired
    if (!*buffer) { fclose(fp); return MCPKG_ERROR_GENERAL; }

    size_t rd = fread(*buffer, 1, *size, fp);
    fclose(fp);
    if (rd != *size) {
        free(*buffer); *buffer = NULL; *size = 0;
        return MCPKG_ERROR_FS;
    }
    (*buffer)[*size] = '\0';
    return MCPKG_ERROR_SUCCESS;
}



