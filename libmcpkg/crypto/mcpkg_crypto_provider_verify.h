/* SPDX-License-Identifier: MIT
 *
 * mcpkg_crypto_provider_verify.h
 * Verify downloaded files against provider-supplied checksums.
 */
#ifndef MCPKG_CRYPTO_PROVIDER_VERIFY_H
#define MCPKG_CRYPTO_PROVIDER_VERIFY_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "mcpkg_crypto_util.h"

MCPKG_BEGIN_DECLS

/*
 * Generic verifier: pass any combination of expected digests as lowercase/upper
 * hex strings (exact length required). We compute only what is provided and
 * return MCPKG_CRYPTO_OK if any match, MCPKG_CRYPTO_ERR_MISMATCH otherwise.
 *
 * If all expected_* are NULL, returns MCPKG_CRYPTO_ERR_ARG.
 *
 * expected lengths:
 *   md5:        32 hex
 *   sha1:       40 hex
 *   sha256:     64 hex
 *   sha512:    128 hex
 *   blake2b32:  64 hex  (BLAKE2b-256)
 */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_verify_file_generic(const char *path,
                                 const char *expected_md5_hex,
                                 const char *expected_sha1_hex,
                                 const char *expected_sha256_hex,
                                 const char *expected_sha512_hex,
                                 const char *expected_blake2b32_hex);

/* Modrinth typically provides sha512 and sha1 (both hex). Either may be NULL. */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_verify_modrinth_file(const char *path,
                                  const char *expected_sha512_hex,
                                  const char *expected_sha1_hex);

/* CurseForge commonly provides md5 (sometimes sha1). Either may be NULL. */
MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_verify_curseforge_file(const char *path,
                                    const char *expected_md5_hex,
                                    const char *expected_sha1_hex);

MCPKG_END_DECLS

#endif /* MCPKG_CRYPTO_CRYPTO_PROVIDER_VERIFY_H */
