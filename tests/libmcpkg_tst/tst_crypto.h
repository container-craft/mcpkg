#ifndef TST_CRYPTO_H
#define TST_CRYPTO_H

#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include <crypto/mcpkg_crypto.h>
#include <fs/mcpkg_fs_file.h>
#include <tst_macros.h>

static inline int hex_eq(const char *a, const char *b)
{
	/* safe even if NULL (only equal if both NULL) */
	if (!a || !b)
		return a == b;
	return strcmp(a, b) == 0;
}

static inline void to_hex(const uint8_t *in, size_t n,
                          char *hex, size_t hex_cap)
{
	/* encode to lowercase hex, NUL-terminated */
	(void)mcpkg_crypto_bin2hex(in, n, hex, hex_cap);
}

static inline int write_tmp(const char *path,
                            const void *data, size_t n)
{
	return mcpkg_fs_write_all(path, data, n, 1 /*overwrite*/)
	       == MCPKG_FS_OK;
}

static void test_version_and_init(void)
{
	const char *v = mcpkg_crypto_version();

	CHECK(v && *v, "crypto version string present");
	CHECK_CRYPTO_OK("crypto init",  mcpkg_crypto_init());
}

static void test_rng(void)
{
	uint8_t a[32], b[32];
	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));

	CHECK_CRYPTO_OK("rng a", mcpkg_crypto_rand(a, sizeof(a)));
	CHECK_CRYPTO_OK("rng b", mcpkg_crypto_rand(b, sizeof(b)));

	/* Not all zeros */
	{
		size_t i;
		int any = 0;
		for (i = 0; i < sizeof(a); i++) if (a[i]) {
				any = 1;
				break;
			}
		CHECK(any, "rng a not all zeros");
	}

	/* Very unlikely to be identical across two calls */
	CHECK(memcmp(a, b, sizeof(a)) != 0, "rng a != rng b");
}

static void test_hex(void)
{
	uint8_t src[] = { 0x00, 0xFF, 0x1A, 0x5c, 0x80 };
	char hex[2 * sizeof(src) + 1];
	uint8_t out[sizeof(src)];

	CHECK_CRYPTO_OK("bin2hex",
	                mcpkg_crypto_bin2hex(src, sizeof(src), hex, sizeof(hex)));
	CHECK(hex_eq(hex, "00ff1a5c80"), "hex lowercase output");

	CHECK_CRYPTO_OK("hex2bin",
	                mcpkg_crypto_hex2bin(hex, out, sizeof(out)));
	CHECK(memcmp(src, out, sizeof(src)) == 0, "hex roundtrip");

	/* invalid length (odd) should parse-fail */
	{
		MCPKG_CRYPTO_ERR e =
		        mcpkg_crypto_hex2bin("abc", out, sizeof(out));
		CHECK(e == MCPKG_CRYPTO_ERR_PARSE ||
		      e == MCPKG_CRYPTO_ERR_ARG, "hex bad length fails");
	}
}

static void test_hash_vectors_buf(void)
{
	/* NIST-style “abc” test vectors (lowercase hex) */
	const char *s = "abc";
	uint8_t md[64];
	char hex[129];

	/* MD5 */
	CHECK_CRYPTO_OK("md5 buf",
	                mcpkg_crypto_md5_buf(s, 3, md));
	to_hex(md, MCPKG_MD5_LEN, hex, sizeof(hex));
	CHECK_STR("md5(abc)",
	          hex, "900150983cd24fb0d6963f7d28e17f72");

	/* SHA1 */
	CHECK_CRYPTO_OK("sha1 buf",
	                mcpkg_crypto_sha1_buf(s, 3, md));
	to_hex(md, MCPKG_SHA1_LEN, hex, sizeof(hex));
	CHECK_STR("sha1(abc)",
	          hex, "a9993e364706816aba3e25717850c26c9cd0d89d");

	/* SHA256 */
	CHECK_CRYPTO_OK("sha256 buf",
	                mcpkg_crypto_sha256_buf(s, 3, md));
	to_hex(md, MCPKG_SHA256_LEN, hex, sizeof(hex));
	CHECK_STR("sha256(abc)",
	          hex, "ba7816bf8f01cfea414140de5dae2223"
	          "b00361a396177a9cb410ff61f20015ad");

	/* SHA512 */
	CHECK_CRYPTO_OK("sha512 buf",
	                mcpkg_crypto_sha512_buf(s, 3, md));
	to_hex(md, MCPKG_SHA512_LEN, hex, sizeof(hex));
	CHECK_STR("sha512(abc)",
	          hex, "ddaf35a193617abacc417349ae204131"
	          "12e6fa4e89a97ea20a9eeee64b55d39a"
	          "2192992a274fc1a836ba3c23a3feebbd"
	          "454d4423643ce80e2a9ac94fa54ca49f");
}

static void test_hash_file_and_all(void)
{
	const char *p = "tst_crypto.tmp";
	const char *content = "hello\n";
	uint8_t md5a[MCPKG_MD5_LEN], md5b[MCPKG_MD5_LEN];

	CHECK(write_tmp(p, content, strlen(content)), "write tmp file");

	CHECK_CRYPTO_OK("md5 file", mcpkg_crypto_md5_file(p, md5a));
	CHECK_CRYPTO_OK("md5 buf",
	                mcpkg_crypto_md5_buf(content, strlen(content), md5b));
	CHECK(memcmp(md5a, md5b, MCPKG_MD5_LEN) == 0, "md5 file==buf");

	/* one-pass all digests */
	{
		uint8_t m1[16], s1[20], s256[32], s512[64];
		CHECK_CRYPTO_OK("hash file all",
		                mcpkg_crypto_hash_file_all(p, m1, s1, s256, s512, NULL));
		/* sanity: re-run one-shot sha256 and compare */
		{
			uint8_t s256b[32];
			CHECK_CRYPTO_OK("sha256 file",
			                mcpkg_crypto_sha256_file(p, s256b));
			CHECK(memcmp(s256, s256b, 32) == 0, "sha256 agree");
		}
	}

	(void)mcpkg_fs_unlink(p);
}
static void test_sign_verify(void)
{
	const uint8_t msg[] = "package index payload";
	uint8_t pk[32], sk[64], sig[64];
	uint8_t wrong_pk[32], sk2[64];

	CHECK_CRYPTO_OK("keygen", mcpkg_crypto_ed25519_keygen(pk, sk));
	CHECK_CRYPTO_OK("sign buf",
	                mcpkg_crypto_ed25519_sign_buf(msg, sizeof(msg) - 1, sk, sig));
	CHECK_CRYPTO_OK("verify buf (pk)",
	                mcpkg_crypto_ed25519_verify_buf_pk(msg, sizeof(msg) - 1, sig, pk));

	// second keypair into sk2/wrong_pk so we don't overwrite sk
	CHECK_CRYPTO_OK("keygen wrong", mcpkg_crypto_ed25519_keygen(wrong_pk, sk2));

	// wrong key should fail
	{
		MCPKG_CRYPTO_ERR e =
		        mcpkg_crypto_ed25519_verify_buf_pk(
		                msg, sizeof(msg) - 1, sig, wrong_pk);
		CHECK(e == MCPKG_CRYPTO_ERR_VERIFY, "verify fails on wrong pk");
	}

	// build the file FIRST, then verify_any()
	CHECK(write_tmp("tst_sig.tmp", msg, sizeof(msg) - 1),
	      "write sig tmp");

	// verify_any: succeed when either of two keys includes the signer */
	{
		uint8_t (*arr)[32] = (uint8_t (*)[32])malloc(2 * 32);
		const uint8_t (*pks)[32] = (const uint8_t (*)[32])arr;
		memcpy(arr[0], pk, 32);
		memcpy(arr[1], wrong_pk, 32);

		CHECK_CRYPTO_OK("verify_any after write",
		                mcpkg_crypto_ed25519_verify_file_any(
		                        "tst_sig.tmp", sig, pks, 2));

		free(arr);
	}

	(void)mcpkg_fs_unlink("tst_sig.tmp");
}

static void test_provider_verify(void)
{
	const char *p = "tst_provider.tmp";
	const char *content = "provider checksum sample";
	uint8_t s1[20], s512[64];
	char s1_hex[41], s512_hex[129];

	CHECK(write_tmp(p, content, strlen(content)), "write provider tmp");

	CHECK_CRYPTO_OK("sha1 file", mcpkg_crypto_sha1_file(p, s1));
	CHECK_CRYPTO_OK("sha512 file", mcpkg_crypto_sha512_file(p, s512));
	to_hex(s1, sizeof(s1), s1_hex, sizeof(s1_hex));
	to_hex(s512, sizeof(s512), s512_hex, sizeof(s512_hex));

	/* Modrinth path (sha512 preferred, sha1 fallback) */
	CHECK_CRYPTO_OK("modrinth verify",
	                mcpkg_crypto_verify_modrinth_file(p, s512_hex, s1_hex));

	/* CurseForge path (md5; we use sha1 here to prove mismatch) */
	{
		MCPKG_CRYPTO_ERR e =
		        mcpkg_crypto_verify_curseforge_file(p, "deadbeef", s1_hex);
		CHECK(e == MCPKG_CRYPTO_ERR_PARSE ||
		      e == MCPKG_CRYPTO_ERR_MISMATCH,
		      "curseforge bad md5 hex rejected");
	}

	/* Generic: correct sha512 should pass; wrong should fail. */
	{
		MCPKG_CRYPTO_ERR e1 = mcpkg_crypto_verify_file_generic(
		                              p, NULL, NULL, NULL, s512_hex, NULL);
		CHECK(e1 == MCPKG_CRYPTO_OK, "generic sha512 ok");

		MCPKG_CRYPTO_ERR e2 = mcpkg_crypto_verify_file_generic(
		                              p, NULL, NULL, NULL, "00", NULL);
		CHECK(e2 == MCPKG_CRYPTO_ERR_PARSE ||
		      e2 == MCPKG_CRYPTO_ERR_MISMATCH,
		      "generic wrong hex rejected");
	}

	(void)mcpkg_fs_unlink(p);
}

static inline void run_tst_crypto(void)
{
	int before = g_tst_fails;

	tst_info("mcpkg crypto: starting...");

	test_version_and_init();
	test_rng();
	test_hex();
	test_hash_vectors_buf();
	test_hash_file_and_all();
	test_sign_verify();
	test_provider_verify();

	if (g_tst_fails == before)
		(void)TST_WRITE(TST_OUT_FD, "mcpkg crypto: OK\n", 17);
}

#endif /* TST_CRYPTO_H */
