// SPDX-License-Identifier: MIT
/*
 * mcpkg_crypto_hex.c
 */

#include "mcpkg_crypto_hex.h"
#include <string.h>
#include <sodium.h>

static int hex_nibble(char c)
{
	if (c >= '0' && c <= '9')
		return (int)(c - '0');
	if (c >= 'a' && c <= 'f')
		return (int)(c - 'a' + 10);
	if (c >= 'A' && c <= 'F')
		return (int)(c - 'A' + 10);
	return -1;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hex2bin(const char *hex, uint8_t *out, size_t out_len)
{
	size_t i;

	if (!hex || (!out && out_len))
		return MCPKG_CRYPTO_ERR_ARG;

	for (i = 0; i < out_len; i++) {
		int hi = hex_nibble(hex[i * 2]);
		int lo = hex_nibble(hex[i * 2 + 1]);

		if (hi < 0 || lo < 0)
			return MCPKG_CRYPTO_ERR_PARSE;

		out[i] = (uint8_t)((hi << 4) | lo);
	}
	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_bin2hex(const uint8_t *in, size_t in_len,
                     char *hex, size_t hex_cap)
{
	static const char hexd[16] = "0123456789abcdef";
	size_t i;

	if ((!in && in_len) || !hex)
		return MCPKG_CRYPTO_ERR_ARG;
	if (hex_cap < (in_len * 2 + 1))
		return MCPKG_CRYPTO_ERR_ARG;

	for (i = 0; i < in_len; i++) {
		hex[i * 2]     = hexd[(in[i] >> 4) & 0xF];
		hex[i * 2 + 1] = hexd[in[i] & 0xF];
	}
	hex[in_len * 2] = '\0';
	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_base64_encode(const uint8_t *in, size_t in_len,
                           char *out, size_t out_cap)
{
	size_t need;

	if ((!in && in_len) || !out)
		return MCPKG_CRYPTO_ERR_ARG;

	need = sodium_base64_ENCODED_LEN(in_len,
	                                 sodium_base64_VARIANT_ORIGINAL);
	if (out_cap < need)
		return MCPKG_CRYPTO_ERR_ARG;

	if (!sodium_bin2base64(out, out_cap, in, in_len,
	                       sodium_base64_VARIANT_ORIGINAL))
		return MCPKG_CRYPTO_ERR_PARSE; /* unexpected */

	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_base64_decode(const char *in,
                           uint8_t *out, size_t out_cap, size_t *out_len)
{
	size_t bin_len = 0;
	int rc;

	if (!in || !out || !out_len)
		return MCPKG_CRYPTO_ERR_ARG;

	rc = sodium_base642bin(out, out_cap, in, strlen(in),
	                       NULL, &bin_len, NULL,
	                       sodium_base64_VARIANT_ORIGINAL);
	if (rc != 0)
		return MCPKG_CRYPTO_ERR_PARSE;

	*out_len = bin_len;
	return MCPKG_CRYPTO_OK;
}
