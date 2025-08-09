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



