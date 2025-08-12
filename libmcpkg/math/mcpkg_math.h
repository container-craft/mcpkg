#ifndef MCPKG_MATH_H
#define MCPKG_MATH_H

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS

/*
 * Tiny (BAD) math/util helpers shared across libmcpkg.
 */

static inline uint64_t mcpkg_math_load64_le(const void *p)
{
    uint64_t v;

    memcpy(&v, p, sizeof(v));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    v = __builtin_bswap64(v);
#endif
    return v;
}

static inline size_t mcpkg_math_min_size(size_t a, size_t b)
{
	return a < b ? a : b;
}

static inline size_t mcpkg_math_max_size(size_t a, size_t b)
{
	return a > b ? a : b;
}

static inline size_t mcpkg_math_clamp_size(size_t v, size_t lo, size_t hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

#define MCPKG_COUNTOF(a) ( sizeof ( a ) / sizeof ( ( a ) [ 0 ] ) )


static inline int mcpkg_math_add_overflow_size(size_t a, size_t b, size_t *out)
{
	if (!out)
		return 1;
	if (SIZE_MAX - a < b)
		return 1;
	*out = a + b;
	return 0;
}

static inline int mcpkg_math_mul_overflow_size(size_t a, size_t b, size_t *out)
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


static inline int mcpkg_math_add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
	if (!out)
		return 1;
	*out = a + b;
	return *out < a;	/* carry => overflow */
}

static inline int mcpkg_math_mul_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
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

static inline int mcpkg_math_is_pow2_size(size_t x)
{
	return x && ((x & (x - 1)) == 0);
}

static inline size_t mcpkg_math_next_pow2_size(size_t x)
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

static inline size_t mcpkg_math_align_down_size(size_t v, size_t align)
{
	return align ? (v & ~(align - 1)) : v;
}

static inline int mcpkg_math_align_up_size(size_t v, size_t align, size_t *out)
{
	size_t add;

	if (!out)
		return 1;
	if (!align)
		return (*out = v, 0);
    if (!mcpkg_math_is_pow2_size(align))
		return 1;

	add = align - 1;
    if (mcpkg_math_add_overflow_size(v, add, out))
		return 1;
	*out &= ~(align - 1);
	return 0;
}


static inline int mcpkg_math_div_ceil_size(size_t n, size_t d, size_t *out)
{
	size_t t;

	if (!out || !d)
		return 1;
    if (mcpkg_math_add_overflow_size(n, d - 1, &t))
		return 1;
	*out = t / d;
	return 0;
}

static inline int mcpkg_math_div_floor_size(size_t n, size_t d, size_t *out)
{
	if (!out || !d)
		return 1;
	*out = n / d;
	return 0;
}

static inline uint32_t mcpkg_math_rotl32(uint32_t x, int r)
{
	return (x << r) | (x >> (32 - r));
}

static inline uint64_t mcpkg_math_rotl64(uint64_t x, int r)
{
	return (x << r) | (x >> (64 - r));
}

static inline uint32_t mcpkg_math_rotr32(uint32_t x, int r)
{
	return (x >> r) | (x << (32 - r));
}

static inline uint64_t mcpkg_math_rotr64(uint64_t x, int r)
{
	return (x >> r) | (x << (64 - r));
}

MCPKG_END_DECLS
#endif /* MCPKG_MATH_H */
