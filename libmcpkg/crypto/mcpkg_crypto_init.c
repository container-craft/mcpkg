// SPDX-License-Identifier: MIT
#include "mcpkg_crypto_init.h"
#include <sodium.h>

MCPKG_API MCPKG_CRYPTO_ERR mcpkg_crypto_init(void)
{
	if (sodium_init() < 0)
		return MCPKG_CRYPTO_ERR_INIT;

	return MCPKG_CRYPTO_OK;
}

MCPKG_API void mcpkg_crypto_shutdown(void)
{
	/* No-op for now. */
}
