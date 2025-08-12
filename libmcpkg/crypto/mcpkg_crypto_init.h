// SPDX-License-Identifier: MIT
#ifndef MCPKG_CRYPTO_INIT_H
#define MCPKG_CRYPTO_INIT_H
#include "crypto/mcpkg_crypto_util.h"
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

MCPKG_API MCPKG_CRYPTO_ERR mcpkg_crypto_init(void);
MCPKG_API void mcpkg_crypto_shutdown(void);

MCPKG_END_DECLS
#endif /* MCPKG_CRYPTO_INIT_H */
