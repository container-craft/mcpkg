// SPDX-License-Identifier: MIT
/*
 * mcpkg_crypto_hash.c
 * Hashing primitives for libmcpkg.
 */

#include "mcpkg_crypto_hash.h"

#include <string.h>
#include <stdio.h>
#include <sodium.h>

#include "fs/mcpkg_fs_file.h"
/* Brad Conte MD5/SHA1 (vendored) */
#include "crypto/third_party/md5/md5sum.h"
#include "crypto/third_party/sha1/sha1.h"

static int sodium_ready(void)
{
	return sodium_init() >= 0;
}

/* Make sure our reserved ctx storage is large enough. */
MCPKG_STATIC_ASSERT(
        sizeof(crypto_hash_sha256_state) <= sizeof(((struct mcpkg_crypto_hash_ctx *)
                        0)->_s256),
        "s256 storage too small");
MCPKG_STATIC_ASSERT(
        sizeof(crypto_hash_sha512_state) <= sizeof(((struct mcpkg_crypto_hash_ctx *)
                        0)->_s512),
        "s512 storage too small");
MCPKG_STATIC_ASSERT(
        sizeof(crypto_generichash_state) <= sizeof(((struct mcpkg_crypto_hash_ctx *)
                        0)->_b2b),
        "b2b storage too small");
MCPKG_STATIC_ASSERT(
        sizeof(MD5_CTX) <= sizeof(((struct mcpkg_crypto_hash_ctx *)0)->_md5),
        "md5 storage too small");
MCPKG_STATIC_ASSERT(
        sizeof(SHA1_CTX) <= sizeof(((struct mcpkg_crypto_hash_ctx *)0)->_sha1),
        "sha1 storage too small");

/* ---- one-shot buffer hashing ------------------------------------------ */

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha256_buf(const void *buf, size_t len, uint8_t out[32])
{
	if ((!buf && len) || !out)
		return MCPKG_CRYPTO_ERR_ARG;
	if (!sodium_ready())
		return MCPKG_CRYPTO_ERR_INIT;

	crypto_hash_sha256(out, (const unsigned char *)buf, (unsigned long long)len);
	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha512_buf(const void *buf, size_t len, uint8_t out[64])
{
	if ((!buf && len) || !out)
		return MCPKG_CRYPTO_ERR_ARG;
	if (!sodium_ready())
		return MCPKG_CRYPTO_ERR_INIT;

	crypto_hash_sha512(out, (const unsigned char *)buf, (unsigned long long)len);
	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_blake2b32_buf(const void *buf, size_t len, uint8_t out[32])
{
	crypto_generichash_state st;

	if ((!buf && len) || !out)
		return MCPKG_CRYPTO_ERR_ARG;
	if (!sodium_ready())
		return MCPKG_CRYPTO_ERR_INIT;

	if (crypto_generichash_init(&st, NULL, 0, MCPKG_BLAKE2B32_LEN) != 0)
		return MCPKG_CRYPTO_ERR_INIT;

	crypto_generichash_update(&st, (const unsigned char *)buf,
	                          (unsigned long long)len);
	crypto_generichash_final(&st, out, MCPKG_BLAKE2B32_LEN);
	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha1_buf(const void *buf, size_t len, uint8_t out[20])
{
	SHA1_CTX c;

	if ((!buf && len) || !out)
		return MCPKG_CRYPTO_ERR_ARG;

	sha1_init(&c);
	sha1_update(&c, (const BYTE *)buf, len);
	sha1_final(&c, out);
	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_md5_buf(const void *buf, size_t len, uint8_t out[16])
{
	MD5_CTX c;

	if ((!buf && len) || !out)
		return MCPKG_CRYPTO_ERR_ARG;

	md5_init(&c);
	md5_update(&c, (const BYTE *)buf, len);
	md5_final(&c, out);
	return MCPKG_CRYPTO_OK;
}


//
//
//   FIXME JOSEPH later on we need to addon to astream buffer from the fs module for files
//   right now the dump is fine as the meta data compressed is like small and so are most jars.
//   but on bigger ones this could cause issues. The hash_file_all() is using the mcpkg_fs_file
//   but again its a full dump. Use hash_file_stream till implemented in mcpkg_fs_file
//




static MCPKG_CRYPTO_ERR hash_file_stream(const char *path,
                uint32_t flags,
                uint8_t *md5,
                uint8_t *sha1,
                uint8_t *sha256,
                uint8_t *sha512,
                uint8_t *b2b32)
{
	FILE *f;
	unsigned char buf[8192];

	crypto_hash_sha256_state s256;
	crypto_hash_sha512_state s512;
	crypto_generichash_state b2b;
	MD5_CTX m5;
	SHA1_CTX s1;

	int do256 = (flags & MCPKG_HASH_SHA256) != 0;
	int do512 = (flags & MCPKG_HASH_SHA512) != 0;
	int doB2  = (flags & MCPKG_HASH_BLAKE2B32) != 0;
	int doMD5 = (flags & MCPKG_HASH_MD5) != 0;
	int doS1  = (flags & MCPKG_HASH_SHA1) != 0;

	size_t n;

	if (!path)
		return MCPKG_CRYPTO_ERR_ARG;

	if ((do256 || do512 || doB2) && !sodium_ready())
		return MCPKG_CRYPTO_ERR_INIT;

	f = fopen(path, "rb");
	if (!f)
		return MCPKG_CRYPTO_ERR_IO;

	if (do256)
		crypto_hash_sha256_init(&s256);
	if (do512)
		crypto_hash_sha512_init(&s512);
	if (doB2)
		crypto_generichash_init(&b2b, NULL, 0, MCPKG_BLAKE2B32_LEN);
	if (doMD5)
		md5_init(&m5);
	if (doS1)
		sha1_init(&s1);

	for (;;) {
		n = fread(buf, 1, sizeof(buf), f);
		if (n > 0) {
			if (do256)
				crypto_hash_sha256_update(&s256, buf, n);
			if (do512)
				crypto_hash_sha512_update(&s512, buf, n);
			if (doB2)
				crypto_generichash_update(&b2b, buf, n);
			if (doMD5)
				md5_update(&m5, buf, n);
			if (doS1)
				sha1_update(&s1, buf, n);
		}
		if (n < sizeof(buf)) {
			if (ferror(f)) {
				fclose(f);
				return MCPKG_CRYPTO_ERR_IO;
			}
			break; /* EOF */
		}
	}

	fclose(f);

	if (do256 && sha256)
		crypto_hash_sha256_final(&s256, sha256);
	if (do512 && sha512)
		crypto_hash_sha512_final(&s512, sha512);
	if (doB2 && b2b32)
		crypto_generichash_final(&b2b, b2b32, MCPKG_BLAKE2B32_LEN);
	if (doMD5 && md5)
		md5_final(&m5, md5);
	if (doS1 && sha1)
		sha1_final(&s1, sha1);

	return MCPKG_CRYPTO_OK;
}


static MCPKG_CRYPTO_ERR hash_file_all(const char *path,
                                      uint32_t flags,
                                      uint8_t *md5,
                                      uint8_t *sha1,
                                      uint8_t *sha256,
                                      uint8_t *sha512,
                                      uint8_t *b2b32)
{
	unsigned char *buf;
	size_t len;
	MCPKG_FS_ERROR fer;

	crypto_hash_sha256_state s256;
	crypto_hash_sha512_state s512;
	crypto_generichash_state b2b;
	MD5_CTX m5;
	SHA1_CTX s1;

	int do256 = (flags & MCPKG_HASH_SHA256) != 0;
	int do512 = (flags & MCPKG_HASH_SHA512) != 0;
	int doB2  = (flags & MCPKG_HASH_BLAKE2B32) != 0;
	int doMD5 = (flags & MCPKG_HASH_MD5) != 0;
	int doS1  = (flags & MCPKG_HASH_SHA1) != 0;

	if (!path)
		return MCPKG_CRYPTO_ERR_ARG;

	if ((do256 || do512 || doB2) && sodium_init() < 0)
		return MCPKG_CRYPTO_ERR_INIT;

	fer = mcpkg_fs_read_all(path, &buf, &len);
	if (fer != MCPKG_FS_OK)
		return MCPKG_CRYPTO_ERR_IO;

	if (do256) {
		crypto_hash_sha256_init(&s256);
		crypto_hash_sha256_update(&s256, buf, len);
		crypto_hash_sha256_final(&s256, sha256);
	}
	if (do512) {
		crypto_hash_sha512_init(&s512);
		crypto_hash_sha512_update(&s512, buf, len);
		crypto_hash_sha512_final(&s512, sha512);
	}
	if (doB2) {
		crypto_generichash_init(&b2b, NULL, 0, MCPKG_BLAKE2B32_LEN);
		crypto_generichash_update(&b2b, buf, len);
		crypto_generichash_final(&b2b, b2b32, MCPKG_BLAKE2B32_LEN);
	}
	if (doMD5) {
		md5_init(&m5);
		md5_update(&m5, buf, len);
		md5_final(&m5, md5);
	}
	if (doS1) {
		sha1_init(&s1);
		sha1_update(&s1, buf, len);
		sha1_final(&s1, sha1);
	}

	free(buf);
	return MCPKG_CRYPTO_OK;
}


MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha256_file(const char *path, uint8_t out[32])
{
	if (!out)
		return MCPKG_CRYPTO_ERR_ARG;
	return hash_file_stream(path, MCPKG_HASH_SHA256,
	                        NULL, NULL, out, NULL, NULL);
}


MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha512_file(const char *path, uint8_t out[64])
{
	if (!out)
		return MCPKG_CRYPTO_ERR_ARG;
	return hash_file_stream(path, MCPKG_HASH_SHA512,
	                        NULL, NULL, NULL, out, NULL);
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_blake2b32_file(const char *path, uint8_t out[32])
{
	if (!out)
		return MCPKG_CRYPTO_ERR_ARG;
	return hash_file_stream(path, MCPKG_HASH_BLAKE2B32,
	                        NULL, NULL, NULL, NULL, out);
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_sha1_file(const char *path, uint8_t out[20])
{
	if (!out)
		return MCPKG_CRYPTO_ERR_ARG;
	return hash_file_stream(path, MCPKG_HASH_SHA1,
	                        NULL, out, NULL, NULL, NULL);
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_md5_file(const char *path, uint8_t out[16])
{
	if (!out)
		return MCPKG_CRYPTO_ERR_ARG;
	return hash_file_stream(path, MCPKG_HASH_MD5,
	                        out, NULL, NULL, NULL, NULL);
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_init(struct mcpkg_crypto_hash_ctx *ctx, uint32_t flags)
{
	if (!ctx)
		return MCPKG_CRYPTO_ERR_ARG;

	memset(ctx, 0, sizeof(*ctx));
	ctx->_flags = flags;

	if ((flags & (MCPKG_HASH_SHA256 | MCPKG_HASH_SHA512 |
	              MCPKG_HASH_BLAKE2B32)) && !sodium_ready())
		return MCPKG_CRYPTO_ERR_INIT;

	if (flags & MCPKG_HASH_SHA256)
		crypto_hash_sha256_init((crypto_hash_sha256_state *)ctx->_s256);
	if (flags & MCPKG_HASH_SHA512)
		crypto_hash_sha512_init((crypto_hash_sha512_state *)ctx->_s512);
	if (flags & MCPKG_HASH_BLAKE2B32)
		crypto_generichash_init((crypto_generichash_state *)ctx->_b2b,
		                        NULL, 0, MCPKG_BLAKE2B32_LEN);
	if (flags & MCPKG_HASH_MD5)
		md5_init((MD5_CTX *)ctx->_md5);
	if (flags & MCPKG_HASH_SHA1)
		sha1_init((SHA1_CTX *)ctx->_sha1);

	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_reset(struct mcpkg_crypto_hash_ctx *ctx)
{
	if (!ctx)
		return MCPKG_CRYPTO_ERR_ARG;

	/* Re-init using the stored flags */
	return mcpkg_crypto_hash_init(ctx, (uint32_t)ctx->_flags);
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_update(struct mcpkg_crypto_hash_ctx *ctx,
                         const void *buf, size_t len)
{
	if (!ctx || (!buf && len))
		return MCPKG_CRYPTO_ERR_ARG;

	if (len == 0)
		return MCPKG_CRYPTO_OK;

	if (ctx->_flags & MCPKG_HASH_SHA256)
		crypto_hash_sha256_update(
		        (crypto_hash_sha256_state *)ctx->_s256,
		        (const unsigned char *)buf, (unsigned long long)len);

	if (ctx->_flags & MCPKG_HASH_SHA512)
		crypto_hash_sha512_update(
		        (crypto_hash_sha512_state *)ctx->_s512,
		        (const unsigned char *)buf, (unsigned long long)len);

	if (ctx->_flags & MCPKG_HASH_BLAKE2B32)
		crypto_generichash_update(
		        (crypto_generichash_state *)ctx->_b2b,
		        (const unsigned char *)buf, (unsigned long long)len);

	if (ctx->_flags & MCPKG_HASH_MD5)
		md5_update((MD5_CTX *)ctx->_md5, (const BYTE *)buf, len);

	if (ctx->_flags & MCPKG_HASH_SHA1)
		sha1_update((SHA1_CTX *)ctx->_sha1, (const BYTE *)buf, len);

	return MCPKG_CRYPTO_OK;
}

MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_final(struct mcpkg_crypto_hash_ctx *ctx,
                        uint8_t *md5, uint8_t *sha1,
                        uint8_t *sha256, uint8_t *sha512,
                        uint8_t *blake2b32)
{
	if (!ctx)
		return MCPKG_CRYPTO_ERR_ARG;

	if ((ctx->_flags & MCPKG_HASH_MD5) && md5)
		md5_final((MD5_CTX *)ctx->_md5, md5);

	if ((ctx->_flags & MCPKG_HASH_SHA1) && sha1)
		sha1_final((SHA1_CTX *)ctx->_sha1, sha1);

	if ((ctx->_flags & MCPKG_HASH_SHA256) && sha256)
		crypto_hash_sha256_final(
		        (crypto_hash_sha256_state *)ctx->_s256, sha256);

	if ((ctx->_flags & MCPKG_HASH_SHA512) && sha512)
		crypto_hash_sha512_final(
		        (crypto_hash_sha512_state *)ctx->_s512, sha512);

	if ((ctx->_flags & MCPKG_HASH_BLAKE2B32) && blake2b32)
		crypto_generichash_final(
		        (crypto_generichash_state *)ctx->_b2b,
		        blake2b32, MCPKG_BLAKE2B32_LEN);

	return MCPKG_CRYPTO_OK;
}


MCPKG_API MCPKG_CRYPTO_ERR
mcpkg_crypto_hash_file_all(const char *path,
                           uint8_t *md5,
                           uint8_t *sha1,
                           uint8_t *sha256,
                           uint8_t *sha512,
                           uint8_t *blake2b32)
{
	uint32_t flags = 0;

	if (!path)
		return MCPKG_CRYPTO_ERR_ARG;

	if (md5)
		flags |= MCPKG_HASH_MD5;
	if (sha1)
		flags |= MCPKG_HASH_SHA1;
	if (sha256)
		flags |= MCPKG_HASH_SHA256;
	if (sha512)
		flags |= MCPKG_HASH_SHA512;
	if (blake2b32)
		flags |= MCPKG_HASH_BLAKE2B32;

	if (!flags)
		return MCPKG_CRYPTO_OK;

	/* stream the file once computing only requested digests */
	return hash_file_stream(path, flags, md5, sha1, sha256, sha512, blake2b32);
	/* or: return hash_file_all(path, flags, md5, sha1, sha256, sha512, blake2b32); */
}
