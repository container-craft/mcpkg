/* libmcpkg/fs/mcpkg_fs_file.c */

#include "fs/mcpkg_fs_file.h"
#include "fs/mcpkg_fs_util.h"

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <direct.h>
#  include <fcntl.h>
#else
#  include <unistd.h>
#  include <fcntl.h>
#  include <stdio.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#endif

#include <zstd.h>

/* ---------- touch ---------- */

MCPKG_FS_ERROR mcpkg_fs_touch(const char *path)
{
#ifdef _WIN32
    HANDLE h;

    if (!path)
        return MCPKG_FS_ERR_NULL_PARAM;

    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return MCPKG_FS_ERR_IO;
    CloseHandle(h);
    return MCPKG_FS_OK;
#else
    int fd;

    if (!path)
        return MCPKG_FS_ERR_NULL_PARAM;

    fd = open(path, O_WRONLY | O_CREAT, MCPKG_FS_FILE_PERM);
    if (fd < 0)
        return (errno == ENOSPC) ? MCPKG_FS_ERR_NOSPC : MCPKG_FS_ERR_IO;

#if defined(HAVE_FUTIMENS) || (_POSIX_C_SOURCE >= 200809L)
    {
        struct timespec ts[2];
        ts[0].tv_sec = ts[1].tv_sec = time(NULL);
        ts[0].tv_nsec = ts[1].tv_nsec = 0;
        (void)futimens(fd, ts);
    }
#endif
    close(fd);
    return MCPKG_FS_OK;
#endif
}

/* ---------- unlink (file or empty dir) ---------- */

MCPKG_FS_ERROR mcpkg_fs_unlink(const char *path)
{
#ifdef _WIN32
    if (!path)
        return MCPKG_FS_ERR_NULL_PARAM;

    if (DeleteFileA(path))
        return MCPKG_FS_OK;
    if (RemoveDirectoryA(path))
        return MCPKG_FS_OK;
    return MCPKG_FS_ERR_IO;
#else
    struct stat st;

    if (!path)
        return MCPKG_FS_ERR_NULL_PARAM;

    if (lstat(path, &st) != 0)
        return (errno == ENOENT) ? MCPKG_FS_ERR_NOT_FOUND
                                 : MCPKG_FS_ERR_IO;

    if (S_ISDIR(st.st_mode))
        return (rmdir(path) == 0) ? MCPKG_FS_OK : MCPKG_FS_ERR_IO;

    return (unlink(path) == 0) ? MCPKG_FS_OK : MCPKG_FS_ERR_IO;
#endif
}

/* ---------- file exists (regular file) ---------- */

int mcpkg_fs_file_exists(const char *path)
{
#ifdef _WIN32
    DWORD attr;

    if (!path)
        return -1;

    attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return 0;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;
#else
    struct stat st;

    if (!path)
        return -1;

    if (lstat(path, &st) != 0)
        return 0;
    return S_ISREG(st.st_mode) ? 1 : 0;
#endif
}

/* ---------- copy file ---------- */

MCPKG_FS_ERROR mcpkg_fs_cp_file(const char *src, const char *dst,
                                int overwrite)
{
#ifdef _WIN32
    DWORD flags;

    if (!src || !dst)
        return MCPKG_FS_ERR_NULL_PARAM;

    flags = overwrite ? 0 : COPY_FILE_FAIL_IF_EXISTS;
    if (CopyFileExA(src, dst, NULL, NULL, NULL, flags))
        return MCPKG_FS_OK;

    return (GetLastError() == ERROR_FILE_EXISTS)
               ? MCPKG_FS_ERR_EXISTS : MCPKG_FS_ERR_IO;
#else
    int in_fd, out_fd;
    char buf[64 * 1024];
    ssize_t n;

    if (!src || !dst)
        return MCPKG_FS_ERR_NULL_PARAM;

    in_fd = open(src, O_RDONLY);
    if (in_fd < 0)
        return (errno == ENOENT) ? MCPKG_FS_ERR_NOT_FOUND
                                 : MCPKG_FS_ERR_IO;

    if (!overwrite) {
        out_fd = open(dst, O_WRONLY | O_CREAT | O_EXCL,
                      MCPKG_FS_FILE_PERM);
        if (out_fd < 0) {
            close(in_fd);
            return (errno == EEXIST) ? MCPKG_FS_ERR_EXISTS
                                     : MCPKG_FS_ERR_IO;
        }
    } else {
        out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC,
                      MCPKG_FS_FILE_PERM);
        if (out_fd < 0) {
            close(in_fd);
            return MCPKG_FS_ERR_IO;
        }
    }

    while ((n = read(in_fd, buf, sizeof buf)) > 0) {
        char *p = buf;
        ssize_t left = n;
        while (left > 0) {
            ssize_t w = write(out_fd, p, (size_t)left);
            if (w <= 0) {
                close(in_fd);
                close(out_fd);
                return MCPKG_FS_ERR_IO;
            }
            p += w;
            left -= w;
        }
    }
    close(in_fd);
    if (close(out_fd) != 0)
        return MCPKG_FS_ERR_IO;

    return (n < 0) ? MCPKG_FS_ERR_IO : MCPKG_FS_OK;
#endif
}

/* ---------- read all ---------- */

MCPKG_FS_ERROR mcpkg_fs_read_all(const char *path,
                                 unsigned char **buf_out, size_t *size_out)
{
    FILE *fp;
    size_t n, rd;
    unsigned char *buf;

    if (!path || !buf_out || !size_out)
        return MCPKG_FS_ERR_NULL_PARAM;

    fp = fopen(path, "rb");
    if (!fp)
        return MCPKG_FS_ERR_IO;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return MCPKG_FS_ERR_IO;
    }
    {
        long end = ftell(fp);
        if (end < 0) {
            fclose(fp);
            return MCPKG_FS_ERR_IO;
        }
        n = (size_t)end;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return MCPKG_FS_ERR_IO;
    }

    buf = (unsigned char *)malloc(n ? n : 1);
    if (!buf) {
        fclose(fp);
        return MCPKG_FS_ERR_OOM;
    }
    rd = fread(buf, 1, n, fp);
    fclose(fp);

    if (rd != n) {
        free(buf);
        return MCPKG_FS_ERR_IO;
    }

    *buf_out = buf;
    *size_out = n;
    return MCPKG_FS_OK;
}

/* ---------- write all ---------- */

MCPKG_FS_ERROR mcpkg_fs_write_all(const char *path,
                                  const void *data, size_t size,
                                  int overwrite)
{
#ifdef _WIN32
    DWORD disp;
    HANDLE h;
    const unsigned char *p;
    size_t left;

    if (!path || (!data && size))
        return MCPKG_FS_ERR_NULL_PARAM;

    disp = overwrite ? CREATE_ALWAYS : CREATE_NEW;
    h = CreateFileA(path, GENERIC_WRITE, 0, NULL, disp,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return overwrite ? MCPKG_FS_ERR_IO : MCPKG_FS_ERR_EXISTS;

    p = (const unsigned char *)data;
    left = size;
    while (left > 0) {
        DWORD wrote = 0;
        DWORD chunk = (DWORD)((left > 0x7fffffffU)
                                   ? 0x7fffffffU : left);
        if (!WriteFile(h, p, chunk, &wrote, NULL)) {
            CloseHandle(h);
            return MCPKG_FS_ERR_IO;
        }
        p += wrote;
        left -= wrote;
    }
    CloseHandle(h);
    return MCPKG_FS_OK;
#else
    int fd;
    const unsigned char *p;
    ssize_t w;
    size_t left;

    if (!path || (!data && size))
        return MCPKG_FS_ERR_NULL_PARAM;

    if (!overwrite) {
        fd = open(path, O_WRONLY | O_CREAT | O_EXCL,
                  MCPKG_FS_FILE_PERM);
        if (fd < 0)
            return (errno == EEXIST) ? MCPKG_FS_ERR_EXISTS
                                     : MCPKG_FS_ERR_IO;
    } else {
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC,
                  MCPKG_FS_FILE_PERM);
        if (fd < 0)
            return MCPKG_FS_ERR_IO;
    }

    p = (const unsigned char *)data;
    left = size;
    while (left > 0) {
        w = write(fd, p, left);
        if (w <= 0) {
            close(fd);
            return MCPKG_FS_ERR_IO;
        }
        p += (size_t)w;
        left -= (size_t)w;
    }
    if (close(fd) != 0)
        return MCPKG_FS_ERR_IO;
    return MCPKG_FS_OK;
#endif
}

/* ---------- write zstd ---------- */

MCPKG_FS_ERROR mcpkg_fs_write_zstd(const char *path, const void *data,
                                   size_t size, int level)
{
    FILE *fp;
    size_t bound, wr;
    void *cbuf;
    size_t clen;

    if (!path || (!data && size))
        return MCPKG_FS_ERR_NULL_PARAM;

    fp = fopen(path, "wb");
    if (!fp)
        return MCPKG_FS_ERR_IO;

    bound = ZSTD_compressBound(size);
    cbuf = malloc(bound);
    if (!cbuf) {
        fclose(fp);
        return MCPKG_FS_ERR_OOM;
    }

    clen = ZSTD_compress(cbuf, bound, data, size, level);
    if (ZSTD_isError(clen)) {
        free(cbuf);
        fclose(fp);
        return MCPKG_FS_ERR_IO;
    }

    wr = fwrite(cbuf, 1, clen, fp);
    free(cbuf);
    if (wr != clen) {
        fclose(fp);
        return MCPKG_FS_ERR_IO;
    }

    if (fclose(fp) != 0)
        return MCPKG_FS_ERR_IO;

    return MCPKG_FS_OK;
}

/* ---------- read zstd ---------- */

static MCPKG_FS_ERROR read_entire_file(const char *path,
                                       unsigned char **buf_out,
                                       size_t *size_out)
{
    return mcpkg_fs_read_all(path, buf_out, size_out);
}

MCPKG_FS_ERROR mcpkg_fs_read_zstd(const char *path,
                                  unsigned char **buf_out, size_t *size_out)
{
    unsigned char *cbuf = NULL;
    size_t csize = 0;
    unsigned long long fsize;
    unsigned char *obuf = NULL;
    size_t osize;
    size_t dec;
    MCPKG_FS_ERROR er;

    if (!path || !buf_out || !size_out)
        return MCPKG_FS_ERR_NULL_PARAM;

    *buf_out = NULL;
    *size_out = 0;

    er = read_entire_file(path, &cbuf, &csize);
    if (er != MCPKG_FS_OK)
        return er;

    fsize = ZSTD_getFrameContentSize(cbuf, csize);
    if (fsize == ZSTD_CONTENTSIZE_ERROR) {
        free(cbuf);
        return MCPKG_FS_ERR_IO; /* not a zstd frame */
    }

    if (fsize != ZSTD_CONTENTSIZE_UNKNOWN) {
        if (fsize > SIZE_MAX) {
            free(cbuf);
            return MCPKG_FS_ERR_OVERFLOW;
        }
        osize = (size_t)fsize;
        obuf = (unsigned char *)malloc(osize ? osize : 1);
        if (!obuf) {
            free(cbuf);
            return MCPKG_FS_ERR_OOM;
        }
        dec = ZSTD_decompress(obuf, osize, cbuf, csize);
        free(cbuf);
        if (ZSTD_isError(dec) || dec != osize) {
            free(obuf);
            return MCPKG_FS_ERR_IO;
        }
        *buf_out = obuf;
        *size_out = osize;
        return MCPKG_FS_OK;
    }

    /* unknown output size: stream + grow */
    {
        ZSTD_DStream *ds = ZSTD_createDStream();
        ZSTD_inBuffer inb;
        ZSTD_outBuffer outb;
        size_t cap = 64 * 1024;
        size_t ret;

        if (!ds) {
            free(cbuf);
            return MCPKG_FS_ERR_OOM;
        }
        ret = ZSTD_initDStream(ds);
        if (ZSTD_isError(ret)) {
            ZSTD_freeDStream(ds);
            free(cbuf);
            return MCPKG_FS_ERR_IO;
        }

        obuf = (unsigned char *)malloc(cap);
        if (!obuf) {
            ZSTD_freeDStream(ds);
            free(cbuf);
            return MCPKG_FS_ERR_OOM;
        }
        inb.src = cbuf;
        inb.size = csize;
        inb.pos = 0;
        outb.dst = obuf;
        outb.size = cap;
        outb.pos = 0;

        while (inb.pos < inb.size) {
            ret = ZSTD_decompressStream(ds, &outb, &inb);
            if (ZSTD_isError(ret)) {
                free(obuf);
                ZSTD_freeDStream(ds);
                free(cbuf);
                return MCPKG_FS_ERR_IO;
            }
            if (outb.pos == outb.size) {
                size_t new_cap = cap * 2;
                unsigned char *nb =
                    (unsigned char *)realloc(obuf, new_cap);
                if (!nb) {
                    free(obuf);
                    ZSTD_freeDStream(ds);
                    free(cbuf);
                    return MCPKG_FS_ERR_OOM;
                }
                obuf = nb;
                outb.dst = obuf;
                outb.size = new_cap;
                outb.pos = cap;
                cap = new_cap;
            }
        }

        ZSTD_freeDStream(ds);
        free(cbuf);

        *buf_out = obuf;
        *size_out = outb.pos;
        return MCPKG_FS_OK;
    }
}

/* ---------- read link target ---------- */

MCPKG_FS_ERROR mcpkg_fs_link_target(const char *link_path,
                                    char *buffer, size_t buf_size,
                                    size_t *out_len)
{
#ifdef _WIN32
    (void)link_path;
    (void)buffer;
    (void)buf_size;
    (void)out_len;
    return MCPKG_FS_ERR_UNSUPPORTED;
#else
    ssize_t n;

    if (!link_path || !buffer || buf_size == 0)
        return MCPKG_FS_ERR_NULL_PARAM;

    n = readlink(link_path, buffer, buf_size);
    if (n < 0)
        return (errno == EINVAL) ? MCPKG_FS_ERR_UNSUPPORTED
                                 : MCPKG_FS_ERR_IO;

    if ((size_t)n < buf_size)
        buffer[n] = '\0';
    else
        return MCPKG_FS_ERR_RANGE; /* truncated */

    if (out_len)
        *out_len = (size_t)n;
    return MCPKG_FS_OK;
#endif
}

/* ---------- symlink (ln -s) ---------- */

MCPKG_FS_ERROR mcpkg_fs_ln_sf(const char *target, const char *link_path,
                              int overwrite)
{
#ifdef _WIN32
    (void)target;
    (void)link_path;
    (void)overwrite;
    return MCPKG_FS_ERR_UNSUPPORTED;
#else
    struct stat st;

    if (!target || !link_path)
        return MCPKG_FS_ERR_NULL_PARAM;

    if (!overwrite) {
        return (symlink(target, link_path) == 0)
        ? MCPKG_FS_OK
        : (errno == EEXIST ? MCPKG_FS_ERR_EXISTS
                           : MCPKG_FS_ERR_IO);
    }

    if (lstat(link_path, &st) == 0) {
        if (S_ISDIR(st.st_mode))
            return MCPKG_FS_ERR_PERM;
        if (unlink(link_path) != 0)
            return MCPKG_FS_ERR_IO;
    }
    return (symlink(target, link_path) == 0)
               ? MCPKG_FS_OK : MCPKG_FS_ERR_IO;
#endif
}
