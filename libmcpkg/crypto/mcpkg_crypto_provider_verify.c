// SPDX-License-Identifier: MIT
/*
 * mcpkg_crypto_provider_verify.c
*/

#include "mcpkg_crypto_provider_verify.h"
#include "mcpkg_crypto_hash.h"
#include "mcpkg_crypto_hex.h"

#include <string.h>
#include <stdlib.h>

/* Internal helper: decode hex if present with exact size, else set flag=0. */
static MCPKG_CRYPTO_ERR decode_hex_opt(const char *hex,
                                       uint8_t *bin, size_t need,
                                       int *out_have)
{
	if (!out_have)
		return MCPKG_CRYPTO_ERR_ARG;

	if (!hex) {
		*out_have = 0;
		return MCPKG_CRYPTO_OK;
	}
	if (strlen(hex) != need * 2)
		return MCPKG_CRYPTO_ERR_PARSE;

	*out_have = 1;
	return mcpkg_crypto_hex2bin(hex, bin, need);
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_verify_file_generic(const char *path,
                                 const char *expected_md5_hex,
                                 const char *expected_sha1_hex,
                                 const char *expected_sha256_hex,
                                 const char *expected_sha512_hex,
                                 const char *expected_blake2b32_hex)
{
	uint8_t want_md5[16], want_sha1[20], want_sha256[32];
	uint8_t want_sha512[64], want_b2[32];
	uint8_t got_md5[16], got_sha1[20], got_sha256[32];
	uint8_t got_sha512[64], got_b2[32];
	int need_md5 = 0, need_sha1 = 0, need_sha256 = 0;
	int need_sha512 = 0, need_b2 = 0;
	MCPKG_CRYPTO_ERR er;

	if (!path)
		return MCPKG_CRYPTO_ERR_ARG;

	er = decode_hex_opt(expected_md5_hex, want_md5, 16, &need_md5);
	if (er != MCPKG_CRYPTO_OK)
		return er;

	er = decode_hex_opt(expected_sha1_hex, want_sha1, 20, &need_sha1);
	if (er != MCPKG_CRYPTO_OK)
		return er;

	er = decode_hex_opt(expected_sha256_hex, want_sha256, 32, &need_sha256);
	if (er != MCPKG_CRYPTO_OK)
		return er;

	er = decode_hex_opt(expected_sha512_hex, want_sha512, 64, &need_sha512);
	if (er != MCPKG_CRYPTO_OK)
		return er;

	er = decode_hex_opt(expected_blake2b32_hex, want_b2, 32, &need_b2);
	if (er != MCPKG_CRYPTO_OK)
		return er;

	if (!need_md5 && !need_sha1 && !need_sha256 && !need_sha512 && !need_b2)
		return MCPKG_CRYPTO_ERR_ARG;

	er = mcpkg_crypto_hash_file_all(path,
	                                need_md5 ? got_md5 : NULL,
	                                need_sha1 ? got_sha1 : NULL,
	                                need_sha256 ? got_sha256 : NULL,
	                                need_sha512 ? got_sha512 : NULL,
	                                need_b2 ? got_b2 : NULL);
	if (er != MCPKG_CRYPTO_OK)
		return er;

	/* Accept if any provided digest matches (constant-time compare). */
	if (need_sha512 && mcpkg_memeq(want_sha512, got_sha512, 64) == 0)
		return MCPKG_CRYPTO_OK;

	if (need_sha256 && mcpkg_memeq(want_sha256, got_sha256, 32) == 0)
		return MCPKG_CRYPTO_OK;

	if (need_b2 && mcpkg_memeq(want_b2, got_b2, 32) == 0)
		return MCPKG_CRYPTO_OK;

	if (need_sha1 && mcpkg_memeq(want_sha1, got_sha1, 20) == 0)
		return MCPKG_CRYPTO_OK;

	if (need_md5 && mcpkg_memeq(want_md5, got_md5, 16) == 0)
		return MCPKG_CRYPTO_OK;

	return MCPKG_CRYPTO_ERR_MISMATCH;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_verify_modrinth_file(const char *path,
                                  const char *expected_sha512_hex,
                                  const char *expected_sha1_hex)
{
	/* Prefer strong hash if both present; generic handles short-circuit. */
	return mcpkg_crypto_verify_file_generic(path,
	                                        NULL,               /* md5 */
	                                        expected_sha1_hex,  /* sha1 */
	                                        NULL,               /* sha256 */
	                                        expected_sha512_hex,/* sha512 */
	                                        NULL);              /* blake2b32 */
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_verify_curseforge_file(const char *path,
                                    const char *expected_md5_hex,
                                    const char *expected_sha1_hex)
{
	return mcpkg_crypto_verify_file_generic(path,
	                                        expected_md5_hex,   /* md5 */
	                                        expected_sha1_hex,  /* sha1 */
	                                        NULL,               /* sha256 */
	                                        NULL,               /* sha512 */
	                                        NULL);              /* blake2b32 */
}
