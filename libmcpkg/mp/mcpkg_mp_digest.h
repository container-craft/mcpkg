#ifndef MCPKG_MP_DIGEST_H
#define MCPKG_MP_DIGEST_H

#include <stddef.h>
#include <stdint.h>

#include "mcpkg_export.h"
#include "mp/mcpkg_mp_util.h"

MCPKG_BEGIN_DECLS

typedef enum {
    MCPKG_DIGEST_SHA1   = 1,
    MCPKG_DIGEST_SHA256 = 2,
    MCPKG_DIGEST_SHA512 = 3,
    MCPKG_DIGEST_MD5SUM
} MCPKG_DIGEST_ALGO;

typedef struct McPkgDigest {
    MCPKG_DIGEST_ALGO algo; /* required */
    char *hex;              /* required, owned NUL-terminated hex */
} McPkgDigest;

/* lifecycle */
MCPKG_API int  mcpkg_digest_init(McPkgDigest *d);
MCPKG_API void mcpkg_digest_free(McPkgDigest *d);
MCPKG_API int  mcpkg_digest_copy(McPkgDigest *dst, const McPkgDigest *src);

/* validation: checks algo and hex length/charset */
MCPKG_API int  mcpkg_digest_validate(const McPkgDigest *d);

/* ---- msgpack: single digest as a map (tagged) ---- */
/* Writes: { 0:"digest", 1:1, 2:algo, 3:hex } */
MCPKG_API int mcpkg_mp_write_digest(struct McPkgMpWriter *w, const McPkgDigest *d);

/* Reads a top-level digest object (root must be that map) */
MCPKG_API int mcpkg_mp_read_digest(const struct McPkgMpReader *r, McPkgDigest *out);

/* ---- msgpack: digest as value under a key ---- */
MCPKG_API int mcpkg_mp_kv_digest(struct McPkgMpWriter *w, int key, const McPkgDigest *d);

/* Reads digest found at integer key in the root map; duplicates output */
MCPKG_API int mcpkg_mp_get_digest_dup(const struct McPkgMpReader *r, int key, McPkgDigest *out, int *found);

/* ---- arrays of digests under a key ---- */
MCPKG_API int mcpkg_mp_kv_digest_list(struct McPkgMpWriter *w, int key,
                                      const McPkgDigest *arr, size_t n);
MCPKG_API int mcpkg_mp_get_digest_list_dup(const struct McPkgMpReader *r, int key,
                                           McPkgDigest **out_arr, size_t *out_n);

MCPKG_END_DECLS
#endif /* MCPKG_MP_DIGEST_H */
