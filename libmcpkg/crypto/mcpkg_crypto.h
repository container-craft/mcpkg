/* SPDX-License-Identifier: MIT
 *
 * mcpkg_crypto.h
 * Umbrella include + version helpers for libmcpkg crypto.
 */
#ifndef MCPKG_CRYPTO_H
#define MCPKG_CRYPTO_H

#include "mcpkg_export.h"

/* Core shared defs (errors, sizes, helpers). */
#include "crypto/mcpkg_crypto_util.h"

/* Submodules. */
#include "crypto/mcpkg_crypto_hash.h"
#include "crypto/mcpkg_crypto_hex.h"
#include "crypto/mcpkg_crypto_init.h"
// # crypto/mcpkg_crypto_io.h
#include "crypto/mcpkg_crypto_provider_verify.h"
#include "crypto/mcpkg_crypto_rand.h"
#include "crypto/mcpkg_crypto_sign.h"
#include "crypto/mcpkg_crypto_util.h"
#include "crypto/mcpkg_sip_hash.h"
#include "crypto/third_party/md5/md5sum.h"
#include "crypto/third_party/sha1/sha1.h"

/* Library semantic version (compile-time). */
#define MCPKG_CRYPTO_VERSION_MAJOR 0
#define MCPKG_CRYPTO_VERSION_MINOR 1
#define MCPKG_CRYPTO_VERSION_PATCH 0

MCPKG_BEGIN_DECLS

/* Return a static NUL-terminated version string "MAJOR.MINOR.PATCH". */
MCPKG_API const char *mcpkg_crypto_version(void);

/* Write out the numeric version triplet. Any pointer may be NULL. */
MCPKG_API void mcpkg_crypto_version_nums(int *major, int *minor, int *patch);

MCPKG_END_DECLS

#endif /* MCPKG_CRYPTO_H */
