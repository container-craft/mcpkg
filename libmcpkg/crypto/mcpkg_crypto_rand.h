/* SPDX-License-Identifier: MIT
 *
 * mcpkg_crypto_rand.h
 * Random utilities for libmcpkg crypto (libsodium backend).
 */
#ifndef MCPKG_CRYPTO_RAND_H
#define MCPKG_CRYPTO_RAND_H

#include <stddef.h>
#include <stdint.h>
#include "mcpkg_export.h"
#include "mcpkg_crypto_util.h"

MCPKG_BEGIN_DECLS

/*
 * Fill `buf` with `len` cryptographically-strong random bytes.
 * Returns MCPKG_CRYPTO_OK on success, negative MCPKG_CRYPTO_ERR on failure.
 *
 * Requires libsodium to be initialized; we defensively call sodium_init()
 * so it is safe even if the caller forgot mcpkg_crypto_init().
 */
MCPKG_API MCPKG_CRYPTO_ERR mcpkg_crypto_rand(void *buf, size_t len);

/*
 * 32-bit random value.
 * Uniform on [0, 2^32-1].
 */
MCPKG_API uint32_t mcpkg_crypto_rand_u32(void);

/*
 * 64-bit random value.
 * Uniform on [0, 2^64-1].
 */
MCPKG_API uint64_t mcpkg_crypto_rand_u64(void);

/*
 * Uniform random in [0, upper) without modulo bias.
 * If upper == 0, returns 0.
 */
MCPKG_API uint32_t mcpkg_crypto_rand_uniform(uint32_t upper);

MCPKG_END_DECLS

#endif /* MCPKG_CRYPTO_RAND_H */
