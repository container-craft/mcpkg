/* SPDX-License-Identifier: MIT
 *
 * mcpkg_crypto_sign.h
 * Ed25519 keygen, detached sign/verify for libmcpkg.
 */
#ifndef MCPKG_CRYPTO_SIGN_H
#define MCPKG_CRYPTO_SIGN_H

#include <stddef.h>
#include <stdint.h>
#include "mcpkg_export.h"
#include "mcpkg_crypto_util.h"

MCPKG_BEGIN_DECLS

// Random keypair (libsodium RNG)
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_keygen(uint8_t pk[32], uint8_t sk[64]);

// Deterministic keypair from 32-byte seed.
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_keygen_seed(const uint8_t seed32[32],
                                 uint8_t pk[32], uint8_t sk[64]);

// Derive public key from secret key (libsodium).
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_sk_to_pk(const uint8_t sk[64], uint8_t pk[32]);

// Compute 32-byte fingerprint = sha256(pk).
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_pk_fingerprint(const uint8_t pk[32], uint8_t fp32[32]);

// Detached signatures over buffers.
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_sign_buf(const void *buf, size_t len,
                              const uint8_t sk[64], uint8_t sig[64]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_verify_buf_pk(const void *buf, size_t len,
                                   const uint8_t sig[64],
                                   const uint8_t pk[32]);

// Files: read whole file via mcpkg_fs_read_all and sign/verify.
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_sign_file(const char *path,
                               const uint8_t sk[64], uint8_t sig[64]);

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_verify_file_pk(const char *path,
                                    const uint8_t sig[64],
                                    const uint8_t pk[32]);

// Verify against any of N public keys; returns OK if any match.
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_verify_file_any(const char *path,
                                     const uint8_t sig[64],
                                     const uint8_t (*pks)[32],
                                     size_t num_pks);

MCPKG_END_DECLS

#endif // MCPKG_CRYPTO_SIGN_H
