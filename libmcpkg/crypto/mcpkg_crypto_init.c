// SPDX-License-Identifier: MIT
/*
 * mcpkg_crypto_init.c
 * One-time initialization for libmcpkg crypto.
 */

#include "mcpkg_crypto_init.h"
#include <sodium.h>

MCPKG_API MCPKG_CRYPTO_ERR mcpkg_crypto_init(void)
{
    if (sodium_init() < 0)
        return MCPKG_CRYPTO_ERR_INIT;

    /* Brad's MD5/SHA1 are per-context; no global init needed. */
    return MCPKG_CRYPTO_OK;
}

MCPKG_API void mcpkg_crypto_shutdown(void)
{
    /* No-op for now. */
}
