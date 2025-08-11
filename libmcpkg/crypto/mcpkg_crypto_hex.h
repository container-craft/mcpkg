/* SPDX-License-Identifier: MIT
 *
 * mcpkg_crypto_hex.h
 * Hex and Base64 helpers for libmcpkg crypto.
 */
#ifndef MCPKG_CRYPTO_HEX_H
#define MCPKG_CRYPTO_HEX_H

#include <stddef.h>
#include <stdint.h>
#include "mcpkg_export.h"
#include "mcpkg_crypto_util.h"

MCPKG_BEGIN_DECLS

        /* ASCII hex (lower/upper) -> binary.
 * hex must be exactly out_len*2 chars. */
            MCPKG_API MCPKG_CRYPTO_ERR
            mcpkg_crypto_hex2bin(const char *hex, uint8_t *out, size_t out_len);

/* binary -> lowercase hex, NUL-terminated.
 * hex_cap must be >= in_len*2 + 1. */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_bin2hex(const uint8_t *in, size_t in_len,
                     char *hex, size_t hex_cap);

/* Base64 (Original alphabet, no wraps). Output is NUL-terminated. */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_base64_encode(const uint8_t *in, size_t in_len,
                           char *out, size_t out_cap);

/* Base64 decode (Original). Writes raw bytes to out and sets *out_len. */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_base64_decode(const char *in,
                           uint8_t *out, size_t out_cap, size_t *out_len);

MCPKG_END_DECLS

#endif /* MCPKG_CRYPTO_HEX_H */
