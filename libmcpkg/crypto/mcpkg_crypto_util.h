// SPDX-License-Identifier: MIT
#ifndef MCPKG_CRYPTO_UTIL_H
#define MCPKG_CRYPTO_UTIL_H

#include <stddef.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS

/* Error codes */
typedef enum {
	MCPKG_CRYPTO_OK              =  0,
	MCPKG_CRYPTO_ERR_IO          = -1,
	MCPKG_CRYPTO_ERR_INIT        = -2,
	MCPKG_CRYPTO_ERR_ARG         = -3,
	MCPKG_CRYPTO_ERR_NOMEM       = -4,
	MCPKG_CRYPTO_ERR_PARSE       = -5,
	MCPKG_CRYPTO_ERR_VERIFY      = -6,
	MCPKG_CRYPTO_ERR_UNSUPPORTED = -7,
	MCPKG_CRYPTO_ERR_MISMATCH    = -8,
	MCPKG_CRYPTO_ERR_SIGN        = -9,
	MCPKG_CRYPTO_ERR_INVALID     = -10
} MCPKG_CRYPTO_ERR;

/* Simple algorithm IDs (informational; not bitmasks) */
typedef enum {
	MCPKG_CRYPTO_ALGO_SHA1     = 1,
	MCPKG_CRYPTO_ALGO_MD5      = 2,
	MCPKG_CRYPTO_ALGO_SHA256   = 3,
	MCPKG_CRYPTO_ALGO_SHA512   = 4,
	MCPKG_CRYPTO_ALGO_BLAKE2B32 = 5,
	MCPKG_CRYPTO_ALGO_ED25519  = 6
} MCPKG_CRYPTO_ALGO_ID;

/* Bitmask capabilities (keep separate from IDs) */
#define MCPKG_HASH_MD5       (1u << 0)
#define MCPKG_HASH_SHA1      (1u << 1)
#define MCPKG_HASH_SHA256    (1u << 2)
#define MCPKG_HASH_SHA512    (1u << 3)
#define MCPKG_HASH_BLAKE2B32 (1u << 4)

/* Digest/Key lengths */
#define MCPKG_MD5_LEN         16u
#define MCPKG_SHA1_LEN        20u
#define MCPKG_SHA256_LEN      32u
#define MCPKG_SHA512_LEN      64u
#define MCPKG_BLAKE2B32_LEN   32u /* blake2b-256 */

#define MCPKG_ED25519_PK_LEN  32u
#define MCPKG_ED25519_SK_LEN  64u
#define MCPKG_ED25519_SIG_LEN 64u

#ifndef MCPKG_ARRAY_LEN
#  define MCPKG_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MCPKG_STATIC_ASSERT
#  define MCPKG_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#endif

/* Map error code to short string ("ok","arg","verify",...) — never NULL. */
MCPKG_API const char *mcpkg_crypto_err_str(int err);

/* Constant-time-ish compare: returns 0 if equal, non-zero otherwise. */
MCPKG_API int mcpkg_memeq(const void *a, const void *b, size_t n);

/* Best-effort secure wipe (tries memset_s / SecureZeroMemory / volatile). */
MCPKG_API void mcpkg_memzero(void *p, size_t n);

MCPKG_END_DECLS
#endif /* MCPKG_CRYPTO_UTIL_H */
