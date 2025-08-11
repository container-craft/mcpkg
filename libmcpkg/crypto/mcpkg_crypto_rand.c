// SPDX-License-Identifier: MIT
#include "mcpkg_crypto_rand.h"
#include <sodium.h>
#include <string.h>

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_rand(void *buf, size_t len)
{
    if ((!buf && len) || sodium_init() < 0)
        return MCPKG_CRYPTO_ERR_INIT;

    if (len == 0)
        return MCPKG_CRYPTO_OK;

    randombytes_buf(buf, len);
    return MCPKG_CRYPTO_OK;
}

MCPKG_API uint32_t
mcpkg_crypto_rand_u32(void)
{
    if (sodium_init() < 0)
        return 0;	/* best-effort fallback */
    return randombytes_random();
}

MCPKG_API uint64_t
mcpkg_crypto_rand_u64(void)
{
    uint64_t v = 0;

    if (sodium_init() < 0)
        return 0;	/* best-effort fallback */

    randombytes_buf(&v, sizeof(v));
    return v;
}

MCPKG_API uint32_t
mcpkg_crypto_rand_uniform(uint32_t upper)
{
    if (upper == 0)
        return 0;

    if (sodium_init() < 0)
        return 0;	/* best-effort fallback */

    return randombytes_uniform(upper);
}
