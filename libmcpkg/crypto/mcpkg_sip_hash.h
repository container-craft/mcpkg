#ifndef MCPKG_SIP_HASH_H
#define MCPKG_SIP_HASH_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "mcpkg_export.h"
#include "math/mcpkg_math.h"
MCPKG_BEGIN_DECLS

typedef struct {
	uint64_t	k0;
	uint64_t	k1;
} mcpkg_siphash_key;

/* ---- SipHash-2-4 (inline) ---- */



static inline uint64_t
mcpkg_siphash24_k(const void *data, size_t len, uint64_t k0, uint64_t k1)
{
	const uint8_t *in = (const uint8_t *)data;
	uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
	uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
	uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
	uint64_t v3 = 0x7465646279746573ULL ^ k1;
	uint64_t m, b;
	size_t i = 0;

#define SIPROUND() \
    do { \
        v0 += v1; v2 += v3; \
        v1 = mcpkg_math_rotl64( v1 , 13 ); \
        v3 = mcpkg_math_rotl64( v3 , 16 ); \
        v1 ^= v0; v3 ^= v2; \
        v0 = mcpkg_math_rotl64( v0 , 32 ); \
        v2 += v1; v0 += v3; \
        v1 = mcpkg_math_rotl64( v1 , 17 ); \
        v3 = mcpkg_math_rotl64( v3 , 21 ); \
        v1 ^= v2; v3 ^= v0; \
        v2 = mcpkg_math_rotl64( v2 , 32 ); \
    } while ( 0 )

    b = ( ( uint64_t ) len ) << 56;

    for ( ; i + 8 <= len; i += 8 ) {
        m = mcpkg_math_load64_le( in + i );
		v3 ^= m;
        SIPROUND ( ) ; SIPROUND ( ) ;
		v0 ^= m;
	}

	m = b;
	switch (len & 7) {
    case 7: m |= ( ( uint64_t ) in [ i + 6 ] ) << 48; /* fallthrough */
    case 6: m |= ( ( uint64_t ) in [ i + 5 ] ) << 40; /* fallthrough */
    case 5: m |= ( ( uint64_t ) in [ i + 4 ] ) << 32; /* fallthrough */
    case 4: m |= ( ( uint64_t ) in [ i + 3 ] ) << 24; /* fallthrough */
    case 3: m |= ( ( uint64_t ) in [ i + 2 ] ) << 16; /* fallthrough */
    case 2: m |= ( ( uint64_t ) in [ i + 1 ] ) << 8;  /* fallthrough */
    case 1: m |= ( ( uint64_t ) in [ i + 0 ] ) << 0;  /* fallthrough */
	default: break;
	}

	v3 ^= m;
    SIPROUND ( ) ; SIPROUND ( ) ;
	v0 ^= m;

	v2 ^= 0xff;
    SIPROUND () ; SIPROUND () ; SIPROUND () ; SIPROUND () ;

#undef SIPROUND
	return v0 ^ v1 ^ v2 ^ v3;
}

static inline uint64_t
mcpkg_siphash24(const void *data, size_t len, mcpkg_siphash_key key)
{
	return mcpkg_siphash24_k(data, len, key.k0, key.k1);
}

static inline uint64_t
mcpkg_siphash24_str(const char *s, mcpkg_siphash_key key)
{
	if (!s)
		return mcpkg_siphash24_k("", 0, key.k0, key.k1);
	return mcpkg_siphash24_k(s, strlen(s), key.k0, key.k1);
}

/* ---- API ---- */

/* Seeds k0/k1 with OS randomness (fallbacks). Implemented in .c */
MCPKG_API void mcpkg_sip_seed(uint64_t *k0, uint64_t *k1);

/* Convenience wrapper for string hashing with raw k0/k1. */
static inline uint64_t
mcpkg_sip_generate(const char *s, uint64_t k0, uint64_t k1)
{
	mcpkg_siphash_key K = { .k0 = k0, .k1 = k1 };
	return mcpkg_siphash24_str(s ? s : "", K);
}

MCPKG_END_DECLS
#endif /* MCPKG_SIP_HASH_H */
