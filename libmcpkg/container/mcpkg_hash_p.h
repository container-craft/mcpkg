#ifndef MCPKG_HASH_P_H
#define MCPKG_HASH_P_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "container/mcpkg_hash.h"
#include "math/mcpkg_math.h"
#include "crypto/mcpkg_sip_hash.h"

#define MCPKG_HASH_LOAD_NUM   82
#define MCPKG_HASH_LOAD_DEN   100
#define MCPKG_HASH_MAX_PROBE  64

#define HST_EMPTY  0u
#define HST_FULL   1u
#define HST_TOMB   2u

// INTERNAL STRUCT DEFINITION (private)
struct McPkgHash {
	char                  **keys;
	uint64_t              *hashes;
	unsigned char         *states;
	unsigned char         *values;
	size_t                 cap;
	size_t                 len;
	size_t                 value_size;
	McPkgHashOps           ops;
	size_t                 max_pairs;
	unsigned long long     max_bytes;
	// SipHash key (128-bit)
	uint64_t               k0;
	uint64_t               k1;
};

static inline char *dup_cstr(const char *s)
{
	size_t n;
	char *p;

	if (!s)
		return NULL;

	n = strlen(s);
	p = malloc(n + 1);
	if (!p)
		return NULL;

	memcpy(p, s, n + 1);
	return p;
}

static inline int loadfactor_exceeded(size_t len, size_t cap)
{
	/* len / cap > 0.82 ? */
	return (len * MCPKG_HASH_LOAD_DEN) > (cap * MCPKG_HASH_LOAD_NUM);
}

static inline uint64_t key_hash(const struct McPkgHash *h, const char *key)
{
	return mcpkg_sip_generate(key ? key : "", h->k0, h->k1);
}

static inline int table_bytes_est(size_t cap, size_t value_size, size_t *out)
{
	size_t a, b, c, d, sum;

	if (!out)
		return 1;

	if (mcpkg_math_mul_overflow_size(cap, sizeof(char *), &a))
		return 1;

	if (mcpkg_math_mul_overflow_size(cap, sizeof(uint64_t), &b))
		return 1;

	if (mcpkg_math_mul_overflow_size(cap, sizeof(unsigned char), &c))
		return 1;

	if (mcpkg_math_mul_overflow_size(cap, value_size, &d))
		return 1;

	if (mcpkg_math_add_overflow_size(a, b, &sum))
		return 1;

	if (mcpkg_math_add_overflow_size(sum, c, &sum))
		return 1;

	if (mcpkg_math_add_overflow_size(sum, d, &sum))
		return 1;

	*out = sum;
	return 0;
}

#endif // MCPKG_HASH_P_H
