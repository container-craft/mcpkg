/* SPDX-License-Identifier: MIT
 *
 * mcpkg_crypto_hash.h
 * Hashing primitives for libmcpkg.
 *
 * Notes:
 * - MD5 and SHA1 are provided for provider compatibility ONLY.
 *   Do not use them for security decisions.
 */
#ifndef MCPKG_CRYPTO_HASH_H
#define MCPKG_CRYPTO_HASH_H

#include <stddef.h>
#include <stdint.h>
#include "mcpkg_export.h"
#include "mcpkg_crypto_util.h"

MCPKG_BEGIN_DECLS

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha256_buf(const void *buf, size_t len, uint8_t out[32]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha512_buf(const void *buf, size_t len, uint8_t out[64]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_blake2b32_buf(const void *buf, size_t len, uint8_t out[32]);

/* Legacy (compat-only) */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha1_buf(const void *buf, size_t len, uint8_t out[20]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_md5_buf(const void *buf, size_t len, uint8_t out[16]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha256_file(const char *path, uint8_t out[32]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha512_file(const char *path, uint8_t out[64]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_blake2b32_file(const char *path, uint8_t out[32]);

/* Legacy Thanks Trump ......JK :P  (compat-only) */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha1_file(const char *path, uint8_t out[20]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_md5_file(const char *path, uint8_t out[16]);

/*
 * Opaque'''''ish context holding internal states for the selected algorithms.
 * You can reuse a ctx for multiple messages by calling _reset() then update().
 */
struct mcpkg_crypto_hash_ctx {
	uint64_t _flags;
	uint8_t  _s256[sizeof(uint64_t) * 16];
	uint8_t  _s512[sizeof(uint64_t) * 32];
	uint8_t  _b2b [512];
	uint8_t  _md5 [128];
	uint8_t  _sha1[128];
};

/* Initialize for a set of algorithms (bitwise OR of MCPKG_HASH_* flags). */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_init(struct mcpkg_crypto_hash_ctx *ctx, uint32_t flags);

/* Optional: reset to start a new message with same enabled algs. */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_reset(struct mcpkg_crypto_hash_ctx *ctx);

/* Feed bytes. Safe to call with len==0 (no-op). */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_update(struct mcpkg_crypto_hash_ctx *ctx,
                         const void *buf, size_t len);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_final(struct mcpkg_crypto_hash_ctx *ctx,
                        uint8_t *md5,
                        uint8_t *sha1,
                        uint8_t *sha256,
                        uint8_t *sha512,
                        uint8_t *blake2b32);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_file_all(const char *path,
                           uint8_t *md5,
                           uint8_t *sha1,
                           uint8_t *sha256,
                           uint8_t *sha512,
                           uint8_t *blake2b32);




MCPKG_END_DECLS

#endif /* MCPKG_CRYPTO_HASH_H */
