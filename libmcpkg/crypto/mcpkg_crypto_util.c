// SPDX-License-Identifier: MIT
/*
 * mcpkg_crypto_util.c
 * Impl for common crypto utilities (errors, ct-compare, secure wipe).
 */

/* Ask for Annex K before including string.h (if present) */
#ifndef __STDC_WANT_LIB_EXT1__
#  define __STDC_WANT_LIB_EXT1__ 1
#endif

#include "crypto/mcpkg_crypto_util.h"

#include <string.h>   /* memset, memset_s */
#include <stdint.h>   /* uint8_t */
#include <stddef.h>   /* size_t */

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h> /* SecureZeroMemory */
#endif

MCPKG_API const char *mcpkg_crypto_err_str(int err)
{
	switch (err) {
		case MCPKG_CRYPTO_OK:
			return "ok";
		case MCPKG_CRYPTO_ERR_IO:
			return "io";
		case MCPKG_CRYPTO_ERR_INIT:
			return "init";
		case MCPKG_CRYPTO_ERR_ARG:
			return "arg";
		case MCPKG_CRYPTO_ERR_NOMEM:
			return "nomem";
		case MCPKG_CRYPTO_ERR_PARSE:
			return "parse";
		case MCPKG_CRYPTO_ERR_VERIFY:
			return "verify";
		case MCPKG_CRYPTO_ERR_UNSUPPORTED:
			return "unsupported";
		case MCPKG_CRYPTO_ERR_MISMATCH:
			return "mismatch";
		default:
			return "unknown";
	}
}

/* Returns 0 if equal, non-zero otherwise. Branchless over data. */
MCPKG_API int mcpkg_memeq(const void *a, const void *b, size_t n)
{
	if (n == 0) return 0;
	if (!a || !b) return 1; /* avoid NULL deref if n>0 */

	const uint8_t *pa = (const uint8_t *)a;
	const uint8_t *pb = (const uint8_t *)b;
	uint8_t diff = 0;
	for (size_t i = 0; i < n; i++) {
		diff |= (uint8_t)(pa[i] ^ pb[i]);
	}
	return (int)diff;
}

MCPKG_API void mcpkg_memzero(void *p, size_t n)
{
	if (!p || n == 0) return;

#if defined(__STDC_LIB_EXT1__)
	(void)memset_s(p, n, 0, n);
#elif defined(_WIN32)
	SecureZeroMemory(p, n);
#else
	volatile uint8_t *vp = (volatile uint8_t *)p;
	while (n--) *vp++ = 0;
#endif
}
