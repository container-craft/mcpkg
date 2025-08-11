/* SPDX-License-Identifier: MIT
 *
 * mcpkg_crypto_utils.c
 * Impl for common crypto utilities (errors, ct-compare, secure wipe).
 */

#include "mcpkg_crypto_util.h"
#include <string.h>   /* memset */
#include <stdint.h>   /* uint8_t */
#include <stddef.h>   /* size_t */


/* Map error code to constant string (never NULL). */
MCPKG_API const char *mcpkg_crypto_err_str(int err)
{
    const char *s = "unknown";
    switch (err) {
    case MCPKG_CRYPTO_OK:              s = "ok"; break;
    case MCPKG_CRYPTO_ERR_IO:          s = "io"; break;
    case MCPKG_CRYPTO_ERR_INIT:        s = "init"; break;
    case MCPKG_CRYPTO_ERR_ARG:         s = "arg"; break;
    case MCPKG_CRYPTO_ERR_NOMEM:       s = "nomem"; break;
    case MCPKG_CRYPTO_ERR_PARSE:       s = "parse"; break;
    case MCPKG_CRYPTO_ERR_VERIFY:      s = "verify"; break;
    case MCPKG_CRYPTO_ERR_UNSUPPORTED: s = "unsupported"; break;
    case MCPKG_CRYPTO_ERR_MISMATCH:    s = "mismatch"; break;
    default:                           s = "unknown"; break;
    }
    return s;
}

/* Constant-time compare: returns 0 if equal, non-zero otherwise. */
MCPKG_API int mcpkg_memeq(const void *a, const void *b, size_t n)
{
    /* Handles NULL with n==0 safely. */
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint8_t diff = 0;
    size_t i;

    for (i = 0; i < n; i++) {
        diff |= (uint8_t)(pa[i] ^ pb[i]);
    }
    /* Return 0 on equal, non-zero otherwise. */
    return (int)diff;
}

/* Best-effort secure wipe that compilers won't optimize away. */
MCPKG_API void mcpkg_memzero(void *p, size_t n)
{
#if defined(__STDC_LIB_EXT1__)
    /* If memset_s is available (C11 Annex K), prefer it. */
    if (p && n) {
        (void)memset_s(p, n, 0, n);
    }
#elif defined(_WIN32)
    /* Windows guarantees not to optimize this away. */
    if (p && n) {
        SecureZeroMemory(p, n);
    }
#else
    /* Portable fallback: volatile pointer prevents optimization. */
    if (p && n) {
        volatile uint8_t *vp = (volatile uint8_t *)p;
        while (n--) {
            *vp++ = 0;
        }
    }
#endif
}

