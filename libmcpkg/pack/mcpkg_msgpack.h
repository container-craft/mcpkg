#ifndef MCPKG_MSGPACK_H
#define MCPKG_MSGPACK_H

#include <stddef.h>
#include <stdint.h>

#include "mcpkg_export.h"

#include "container/mcpkg_str_list.h"

MCPKG_BEGIN_DECLS
// Generic, module-agnostic error codes for msgpack helpers.
typedef enum {
    MCPKG_MP_NO_ERROR        = 0,
    MCPKG_MP_ERR_INVALID_ARG = -1,
    MCPKG_MP_ERR_PARSE       = -2,
    MCPKG_MP_ERR_NO_MEMORY   = -3,
    MCPKG_MP_ERR_IO          = -4,
} MCPKG_MP_ERROR;


// Writer context for building a buffer.
struct McPkgMpWriter {
    void    *impl;     // msgpack-c sbuffer/packer or alt backend
    void    *buf;      // final buffer after finish (malloc'd)
    size_t   len;      // final length after finish
};

// Reader for a single top-level object.
struct McPkgMpReader {
    void           *impl; // msgpack-c unpacked object
    const void     *buf;
    size_t          len;
    void           *root; // cached root object
};

MCPKG_API int mcpkg_mp_writer_init(struct McPkgMpWriter *w);
MCPKG_API void mcpkg_mp_writer_destroy(struct McPkgMpWriter *w);
MCPKG_API int mcpkg_mp_writer_finish(struct McPkgMpWriter *w, void **out_buf,
                                     size_t *out_len);

MCPKG_API int mcpkg_mp_reader_init(struct McPkgMpReader *r,
                                   const void *buf, size_t len);
MCPKG_API void mcpkg_mp_reader_destroy(struct McPkgMpReader *r);

// Map helpers (int-keyed map with tag/version header)
// Begin a map with known key-count (include tag+ver in the count passed).
MCPKG_API int mcpkg_mp_map_begin(struct McPkgMpWriter *w, uint32_t key_count);

// Write Key Value pairs (int key)
MCPKG_API int mcpkg_mp_kv_i32(struct McPkgMpWriter *w, int key, int32_t v);
MCPKG_API int mcpkg_mp_kv_u32(struct McPkgMpWriter *w, int key, uint32_t v);
MCPKG_API int mcpkg_mp_kv_i64(struct McPkgMpWriter *w, int key, int64_t v);
MCPKG_API int mcpkg_mp_kv_str(struct McPkgMpWriter *w, int key, const char *s);
MCPKG_API int mcpkg_mp_kv_bin(struct McPkgMpWriter *w, int key,
                              const void *data, uint32_t len);
MCPKG_API int mcpkg_mp_kv_nil(struct McPkgMpWriter *w, int key);

// Standard header helpers (all structs)
#define MCPKG_MP_K_TAG  0
#define MCPKG_MP_K_VER  1

MCPKG_API int mcpkg_mp_write_header(struct McPkgMpWriter *w,
                                    const char *tag, int version);

// Reader-side: query map by int key (non-failing scans)
MCPKG_API int mcpkg_mp_get_i64(const struct McPkgMpReader *r, int key,
                               int64_t *out, int *found);
MCPKG_API int mcpkg_mp_get_u64(const struct McPkgMpReader *r, int key,
                               uint64_t *out, int *found);
MCPKG_API int mcpkg_mp_get_u32(const struct McPkgMpReader *r, int key,
                               uint32_t *out, int *found);
MCPKG_API int mcpkg_mp_get_str_borrow(const struct McPkgMpReader *r, int key,
                                      const char **out_ptr, size_t *out_len,
                                      int *found);
MCPKG_API int mcpkg_mp_get_bin_borrow(const struct McPkgMpReader *r, int key,
                                      const void **out_ptr, size_t *out_len,
                                      int *found);

// Convenience: verify tag, return version (unknown tag -> PARSE error)
MCPKG_API int mcpkg_mp_expect_tag(const struct McPkgMpReader *r,
                                  const char *expected_tag, int *out_version);

// Small helpers for common containers (used by mc versions, etc.)
struct McPkgStrList;
MCPKG_API int mcpkg_mp_kv_strlist(struct McPkgMpWriter *w, int key,
                                  const struct McPkgStringList *sl);
MCPKG_API int mcpkg_mp_get_strlist_dup(const struct McPkgMpReader *r, int key,
                                       struct McPkgStringList **out_sl);

MCPKG_END_DECLS
#endif /* MCPKG_MSGPACK_H */
