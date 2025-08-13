// SPDX-License-Identifier: MIT
/*
 * mcpkg_crypto.c
 * Umbrella version helpers for libmcpkg crypto.
 */

#include "mcpkg_crypto.h"

#define MCPKG_CRYPTO_STR_HELPER(x) #x
#define MCPKG_CRYPTO_STR(x) MCPKG_CRYPTO_STR_HELPER(x)

static const char mcpkg_crypto_version_str[] =
        MCPKG_CRYPTO_STR(MCPKG_CRYPTO_VERSION_MAJOR) "."
        MCPKG_CRYPTO_STR(MCPKG_CRYPTO_VERSION_MINOR) "."
        MCPKG_CRYPTO_STR(MCPKG_CRYPTO_VERSION_PATCH);

MCPKG_API const char *mcpkg_crypto_version(void)
{
	return mcpkg_crypto_version_str;
}

MCPKG_API void mcpkg_crypto_version_nums(int *major, int *minor, int *patch)
{
	if (major)
		*major = MCPKG_CRYPTO_VERSION_MAJOR;
	if (minor)
		*minor = MCPKG_CRYPTO_VERSION_MINOR;
	if (patch)
		*patch = MCPKG_CRYPTO_VERSION_PATCH;
}
