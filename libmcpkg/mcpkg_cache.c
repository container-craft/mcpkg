#include "mcpkg_cache.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>      /* strcasestr, strcasecmp */
#include <stdio.h>
#include <zstd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "utils/array_helper.h"
#include "utils/mcpkg_fs.h"
#include "utils/code_names.h"

/* ----------------- Helpers ----------------- */

static int mcpkg_cache_read_file(const char *path, char **buffer, size_t *size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return MCPKG_ERROR_FS;

    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return MCPKG_ERROR_FS; }
    long end = ftell(fp);
    if (end < 0) { fclose(fp); return MCPKG_ERROR_FS; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return MCPKG_ERROR_FS; }

    *size = (size_t)end;
    *buffer = (char *)malloc(*size ? *size : 1);
    if (!*buffer) { fclose(fp); return MCPKG_ERROR_GENERAL; }

    size_t rd = fread(*buffer, 1, *size, fp);
    fclose(fp);
    if (rd != *size) { free(*buffer); *buffer = NULL; *size = 0; return MCPKG_ERROR_FS; }
    return MCPKG_ERROR_SUCCESS;
}

/* Streaming Zstd decompress into a growable buffer. */
static int zstd_decompress_dynamic(const void *src, size_t srcSize, char **out, size_t *outSize) {
    int ret = MCPKG_ERROR_GENERAL;
    *out = NULL; *outSize = 0;

    ZSTD_DCtx *dctx = ZSTD_createDCtx();
    if (!dctx) return MCPKG_ERROR_GENERAL;

    size_t cap = 1 << 16; /* start with 64 KiB */
    char *dst = (char *)malloc(cap);
    if (!dst) { ZSTD_freeDCtx(dctx); return MCPKG_ERROR_GENERAL; }

    ZSTD_inBuffer in = { src, srcSize, 0 };
    ZSTD_outBuffer outBuf = { dst, cap, 0 };

    while (in.pos < in.size) {
        size_t r = ZSTD_decompressStream(dctx, &outBuf, &in);
        if (ZSTD_isError(r)) { ret = MCPKG_ERROR_GENERAL; goto done; }
        if (outBuf.pos == outBuf.size) {
            /* grow */
            size_t newCap = cap << 1;
            char *tmp = (char *)realloc(dst, newCap);
            if (!tmp) { ret = MCPKG_ERROR_GENERAL; goto done; }
            dst = tmp; cap = newCap;
            outBuf.dst = dst; outBuf.size = cap; /* keep pos */
        }
    }

    *out = dst;
    *outSize = outBuf.pos;
    ret = MCPKG_ERROR_SUCCESS;
done:
    if (ret != MCPKG_ERROR_SUCCESS) free(dst);
    ZSTD_freeDCtx(dctx);
    return ret;
}

static int decompress_zstd_file_buffer(const char *inBuf, size_t inSize, char **outBuf, size_t *outSize) {
    /* Try to get exact frame size first */
    unsigned long long frameSize = ZSTD_getFrameContentSize(inBuf, inSize);
    if (frameSize == ZSTD_CONTENTSIZE_ERROR) {
        /* Not a valid zstd frame */
        return MCPKG_ERROR_PARSE;
    } else if (frameSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        /* Fall back to streaming */
        return zstd_decompress_dynamic(inBuf, inSize, outBuf, outSize);
    } else {
        /* Exact size known */
        size_t dstSize = (size_t)frameSize;
        char *dst = (char *)malloc(dstSize ? dstSize : 1);
        if (!dst) return MCPKG_ERROR_GENERAL;

        size_t dec = ZSTD_decompress(dst, dstSize, inBuf, inSize);
        if (ZSTD_isError(dec)) {
            free(dst);
            return MCPKG_ERROR_PARSE;
        }
        *outBuf = dst;
        *outSize = dec;
        return MCPKG_ERROR_SUCCESS;
    }
}

static int unpack_all_mods_from_buffer(McPkgCache *cache, const char *buffer, size_t size) {
    msgpack_unpacked unpacked;
    msgpack_unpacked_init(&unpacked);
    size_t offset = 0;

    McPkgInfoEntry **temp_mods = NULL;
    size_t temp_count = 0, temp_capacity = 0;

    while (msgpack_unpack_next(&unpacked, buffer, size, &offset)) {
        if (unpacked.data.type != MSGPACK_OBJECT_MAP)
            continue;

        if (temp_count == temp_capacity) {
            size_t new_cap = temp_capacity ? (temp_capacity << 1) : 4;
            McPkgInfoEntry **tmp = (McPkgInfoEntry **)realloc(temp_mods, new_cap * sizeof(*tmp));
            if (!tmp) { /* bail out: free what we already built */
                for (size_t i = 0; i < temp_count; ++i) mcpkg_info_entry_free(temp_mods[i]);
                free(temp_mods);
                msgpack_unpacked_destroy(&unpacked);
                return MCPKG_ERROR_GENERAL;
            }
            temp_mods = tmp;
            temp_capacity = new_cap;
        }

        McPkgInfoEntry *entry = mcpkg_info_entry_new();
        if (!entry) {
            for (size_t i = 0; i < temp_count; ++i) mcpkg_info_entry_free(temp_mods[i]);
            free(temp_mods);
            msgpack_unpacked_destroy(&unpacked);
            return MCPKG_ERROR_GENERAL;
        }

        if (mcpkg_info_entry_unpack(&unpacked.data, entry) == 0) {
            temp_mods[temp_count++] = entry;
        } else {
            mcpkg_info_entry_free(entry);
        }
    }

    msgpack_unpacked_destroy(&unpacked);

    cache->mods = temp_mods;
    cache->mods_count = temp_count;
    return MCPKG_ERROR_SUCCESS;
}


McPkgCache *mcpkg_cache_new(void) {
    McPkgCache *cache = (McPkgCache *)calloc(1, sizeof(McPkgCache));
    if (!cache)
        return NULL;
    return cache;
}

void mcpkg_cache_free(McPkgCache *cache) {
    if (!cache) return;
    free(cache->base_path);
    if (cache->mods) {
        for (size_t i = 0; i < cache->mods_count; ++i) mcpkg_info_entry_free(cache->mods[i]);
        free(cache->mods);
    }
    free(cache);
}

int mcpkg_cache_load(McPkgCache *cache, const char *mod_loader, const char *version) {
    if (!cache || !mod_loader || !version) return MCPKG_ERROR_PARSE;

    int rc = MCPKG_ERROR_GENERAL;
    char *compressed_path = NULL;
    char *uncompressed_path = NULL;
    char *data = NULL;
    size_t size = 0;

    /* Resolve cache root and codename */
    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root) cache_root = MCPKG_CACHE;

    const char *codename = codename_for_version(version);
    if (!codename) return MCPKG_ERROR_VERSION_MISMATCH;

    /* Build and store base_path on cache */
    free(cache->base_path); cache->base_path = NULL;
    if (asprintf(&cache->base_path, "%s/%s/%s/%s", cache_root, mod_loader, codename, version) < 0 || !cache->base_path) {
        rc = MCPKG_ERROR_GENERAL;
        goto done;
    }

    /* Build full paths */
    if (asprintf(&compressed_path,   "%s/Packages.info.zstd", cache->base_path) < 0 || !compressed_path)
        goto done;
    if (asprintf(&uncompressed_path, "%s/Packages.info",      cache->base_path) < 0 || !uncompressed_path)
        goto done;

    if (access(compressed_path, F_OK) == 0) {
        /* Read compressed */
        rc = mcpkg_cache_read_file(compressed_path, &data, &size);
        if (rc != MCPKG_ERROR_SUCCESS)
            goto done;

        /* Decompress */
        {
            char *decompressed = NULL; size_t dsz = 0;
            rc = decompress_zstd_file_buffer(data, size, &decompressed, &dsz);
            free(data); data = NULL;
            if (rc != MCPKG_ERROR_SUCCESS) {
                if (decompressed) free(decompressed);
                goto done;
            }

            /* Unpack */
            rc = unpack_all_mods_from_buffer(cache, decompressed, dsz);
            free(decompressed);
            if (rc != MCPKG_ERROR_SUCCESS)
                goto done;
        }

    } else if (access(uncompressed_path, F_OK) == 0) {
        /* Read plain */
        rc = mcpkg_cache_read_file(uncompressed_path, &data, &size);
        if (rc != MCPKG_ERROR_SUCCESS)
            goto done;

        /* Unpack */
        rc = unpack_all_mods_from_buffer(cache, data, size);
        free(data); data = NULL;
        if (rc != MCPKG_ERROR_SUCCESS)
            goto done;

    } else {
        rc = MCPKG_ERROR_NOT_FOUND;
        goto done;
    }

    rc = MCPKG_ERROR_SUCCESS;

done:
    free(compressed_path);
    free(uncompressed_path);
    if (data) free(data);
    return rc;
}


McPkgInfoEntry **mcpkg_cache_search(McPkgCache *cache, const char *package, size_t *matches_count) {
    if (matches_count) *matches_count = 0;
    if (!cache || !cache->mods || !package || !matches_count) return NULL;

    McPkgInfoEntry **matches = NULL;
    size_t cnt = 0;

    for (size_t i = 0; i < cache->mods_count; ++i) {
        McPkgInfoEntry *e = cache->mods[i];
        if (!e) continue;
        if ((e->name   && strstr(e->name, package)) ||
            (e->title  && strstr(e->title, package))) {
            void *tmp = realloc(matches, (cnt + 1) * sizeof(*matches));
            if (!tmp) {
                free(matches);
                *matches_count = 0;
                return NULL;
            }
            matches = (McPkgInfoEntry **)tmp;
            matches[cnt++] = e;
        }
    }

    *matches_count = cnt;
    return matches;
}

char *mcpkg_cache_show(McPkgCache *cache, const char *package) {
    if (!cache || !cache->mods || !package)
        return strdup("");

    for (size_t i = 0; i < cache->mods_count; ++i) {
        McPkgInfoEntry *e = cache->mods[i];
        if (e && e->name && strcasecmp(e->name, package) == 0) {
            return mcpkg_info_entry_to_string(e);
        }
    }
    return strdup("");
}
