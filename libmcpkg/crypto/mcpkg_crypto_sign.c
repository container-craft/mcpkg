// SPDX-License-Identifier: MIT
/*
 * mcpkg_crypto_sign.c
 * Ed25519 detached signatures (libsodium).
 */
#include "mcpkg_crypto_sign.h"

#include <sodium.h>

#include <stdlib.h>
#include <string.h>

#include "mcpkg_crypto_hash.h"
#include "fs/mcpkg_fs_file.h"

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_keygen(uint8_t pk[32], uint8_t sk[64])
{
	if (!pk || !sk)
		return MCPKG_CRYPTO_ERR_ARG;
	if (sodium_init() < 0)
		return MCPKG_CRYPTO_ERR_INIT;

	if (crypto_sign_ed25519_keypair(pk, sk) != 0)
		return MCPKG_CRYPTO_ERR_INIT;

	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_keygen_seed(const uint8_t seed32[32],
                                 uint8_t pk[32], uint8_t sk[64])
{
	if (!seed32 || !pk || !sk)
		return MCPKG_CRYPTO_ERR_ARG;
	if (sodium_init() < 0)
		return MCPKG_CRYPTO_ERR_INIT;

	if (crypto_sign_ed25519_seed_keypair(pk, sk, seed32) != 0)
		return MCPKG_CRYPTO_ERR_INIT;

	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_sk_to_pk(const uint8_t sk[64], uint8_t pk[32])
{
	if (!sk || !pk)
		return MCPKG_CRYPTO_ERR_ARG;
	if (sodium_init() < 0)
		return MCPKG_CRYPTO_ERR_INIT;

	if (crypto_sign_ed25519_sk_to_pk(pk, sk) != 0)
		return MCPKG_CRYPTO_ERR_PARSE;

	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_pk_fingerprint(const uint8_t pk[32], uint8_t fp32[32])
{
	if (!pk || !fp32)
		return MCPKG_CRYPTO_ERR_ARG;

	/* sha256(pk) */
	return mcpkg_crypto_sha256_buf(pk, 32, fp32);
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_sign_buf(const void *buf, size_t len,
                              const uint8_t sk[64], uint8_t sig[64])
{
	unsigned long long siglen;

	if ((!buf && len) || !sk || !sig)
		return MCPKG_CRYPTO_ERR_ARG;
	if (sodium_init() < 0)
		return MCPKG_CRYPTO_ERR_INIT;

	if (crypto_sign_ed25519_detached(sig, &siglen,
	                                 (const unsigned char *)buf,
	                                 (unsigned long long)len,
	                                 sk) != 0)
		return MCPKG_CRYPTO_ERR_VERIFY; /* unexpected */
	if (siglen != 64)
		return MCPKG_CRYPTO_ERR_VERIFY;

	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_verify_buf_pk(const void *buf, size_t len,
                                   const uint8_t sig[64],
                                   const uint8_t pk[32])
{
	if ((!buf && len) || !sig || !pk)
		return MCPKG_CRYPTO_ERR_ARG;
	if (sodium_init() < 0)
		return MCPKG_CRYPTO_ERR_INIT;

	if (crypto_sign_ed25519_verify_detached(sig,
	                                        (const unsigned char *)buf,
	                                        (unsigned long long)len,
	                                        pk) != 0)
		return MCPKG_CRYPTO_ERR_VERIFY;

	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_sign_file(const char *path,
                               const uint8_t sk[64], uint8_t sig[64])
{
	unsigned char *buf;
	size_t len;
	MCPKG_FS_ERROR fer;
	MCPKG_CRYPTO_ERR er;

	if (!path || !sk || !sig)
		return MCPKG_CRYPTO_ERR_ARG;

	fer = mcpkg_fs_read_all(path, &buf, &len);
	if (fer != MCPKG_FS_OK)
		return MCPKG_CRYPTO_ERR_IO;

	er = mcpkg_crypto_ed25519_sign_buf(buf, len, sk, sig);
	mcpkg_memzero(buf, len);
	free(buf);
	return er;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_verify_file_pk(const char *path,
                                    const uint8_t sig[64],
                                    const uint8_t pk[32])
{
	unsigned char *buf;
	size_t len;
	MCPKG_FS_ERROR fer;
	MCPKG_CRYPTO_ERR er;

	if (!path || !sig || !pk)
		return MCPKG_CRYPTO_ERR_ARG;

	fer = mcpkg_fs_read_all(path, &buf, &len);
	if (fer != MCPKG_FS_OK)
		return MCPKG_CRYPTO_ERR_IO;

	er = mcpkg_crypto_ed25519_verify_buf_pk(buf, len, sig, pk);
	mcpkg_memzero(buf, len);
	free(buf);
	return er;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_ed25519_verify_file_any(const char *path,
                                     const uint8_t sig[64],
                                     const uint8_t (*pks)[32],
                                     size_t num_pks)
{
	size_t i;
	unsigned char *buf;
	size_t len;
	MCPKG_FS_ERROR fer;

	if (!path || !sig || (!pks && num_pks))
		return MCPKG_CRYPTO_ERR_ARG;

	if (num_pks == 0)
		return MCPKG_CRYPTO_ERR_ARG;

	fer = mcpkg_fs_read_all(path, &buf, &len);
	if (fer != MCPKG_FS_OK)
		return MCPKG_CRYPTO_ERR_IO;

	for (i = 0; i < num_pks; i++) {
		if (crypto_sign_ed25519_verify_detached(
		            sig, buf, (unsigned long long)len, pks[i]) == 0) {
			mcpkg_memzero(buf, len);
			free(buf);
			return MCPKG_CRYPTO_OK;
		}
	}

	mcpkg_memzero(buf, len);
	free(buf);
	return MCPKG_CRYPTO_ERR_VERIFY;
}
