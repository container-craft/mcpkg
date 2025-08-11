#ifndef MCPKG_MATH_H
#define MCPKG_MATH_H

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS

/*
 * Tiny math/util helpers shared across libmcpkg.
 * Style: snug braces, tabs=8, early returns, ~80 cols.
 *
 * Notes:
 * - All overflow checkers return 1 on overflow, 0 on success.
 * - Alignment helpers expect power-of-two alignments.
 */

/* ----- basic min/max/clamp ----- */

static inline size_t mcpkg_min_size(size_t a, size_t b)
{
	return a < b ? a : b;
}

static inline size_t mcpkg_max_size(size_t a, size_t b)
{
	return a > b ? a : b;
}

static inline size_t mcpkg_clamp_size(size_t v, size_t lo, size_t hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

/* count of compile-time arrays */
#define MCPKG_COUNTOF(a) (sizeof(a) / sizeof((a)[0]))

/* ----- overflow-safe arithmetic (size_t) ----- */

static inline int mcpkg_add_overflow_size(size_t a, size_t b, size_t *out)
{
	if (!out)
		return 1;
	if (SIZE_MAX - a < b)
		return 1;
	*out = a + b;
	return 0;
}

static inline int mcpkg_mul_overflow_size(size_t a, size_t b, size_t *out)
{
	if (!out)
		return 1;
	if (!a || !b) {
		*out = 0;
		return 0;
	}
	if (SIZE_MAX / a < b)
		return 1;
	*out = a * b;
	return 0;
}

/* u64 variants (handy for byte math) */

static inline int mcpkg_add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
	if (!out)
		return 1;
	*out = a + b;
	return *out < a;	/* carry => overflow */
}

static inline int mcpkg_mul_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
	if (!out)
		return 1;
#if defined(__SIZEOF_INT128__)
	__uint128_t p = ( (__uint128_t)a ) * ( (__uint128_t)b );
	*out = (uint64_t)p;
	return (p >> 64) != 0;
#else
	/* portable fallback: check by division */
	if (!a || !b) {
		*out = 0;
		return 0;
	}
	if (a > UINT64_MAX / b)
		return 1;
	*out = a * b;
	return 0;
#endif
}

/* ----- alignment & power-of-two helpers ----- */

static inline int mcpkg_is_pow2_size(size_t x)
{
	return x && ((x & (x - 1)) == 0);
}

/* next power of two (min 1). clamps at highest bit on overflow. */
static inline size_t mcpkg_next_pow2_size(size_t x)
{
	if (x <= 1)
		return 1;
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
#if SIZE_MAX > 0xFFFFu
	x |= x >> 16;
#endif
#if SIZE_MAX > 0xFFFFFFFFu
	x |= x >> 32;
#endif
	x++;
	return x ? x : (SIZE_MAX & ~(SIZE_MAX >> 1)); /* clamp */
}

/* round v up/down to alignment (power-of-two align). overflow-safe up. */
static inline size_t mcpkg_align_down_size(size_t v, size_t align)
{
	return align ? (v & ~(align - 1)) : v;
}

static inline int mcpkg_align_up_size(size_t v, size_t align, size_t *out)
{
	size_t add;

	if (!out)
		return 1;
	if (!align)
		return (*out = v, 0);
	if (!mcpkg_is_pow2_size(align))
		return 1;

	add = align - 1;
	if (mcpkg_add_overflow_size(v, add, out))
		return 1;
	*out &= ~(align - 1);
	return 0;
}

/* ----- division helpers ----- */

/* ceil(n / d), with d>0; returns 1 on overflow/invalid, 0 on success. */
static inline int mcpkg_div_ceil_size(size_t n, size_t d, size_t *out)
{
	size_t t;

	if (!out || !d)
		return 1;
	if (mcpkg_add_overflow_size(n, d - 1, &t))
		return 1;
	*out = t / d;
	return 0;
}

/* floor(n / d), with d>0; trivial but keeps symmetry with ceil. */
static inline int mcpkg_div_floor_size(size_t n, size_t d, size_t *out)
{
	if (!out || !d)
		return 1;
	*out = n / d;
	return 0;
}

/* ----- bit rotates (useful for hashing) ----- */

static inline uint32_t mcpkg_rotl32(uint32_t x, int r)
{
	return (x << r) | (x >> (32 - r));
}

static inline uint64_t mcpkg_rotl64(uint64_t x, int r)
{
	return (x << r) | (x >> (64 - r));
}

static inline uint32_t mcpkg_rotr32(uint32_t x, int r)
{
	return (x >> r) | (x << (32 - r));
}

static inline uint64_t mcpkg_rotr64(uint64_t x, int r)
{
	return (x >> r) | (x << (64 - r));
}

MCPKG_END_DECLS
#endif /* MCPKG_MATH_H */
