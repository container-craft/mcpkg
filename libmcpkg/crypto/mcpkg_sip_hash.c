#include "crypto/mcpkg_sip_hash.h"

#include <stdint.h>
#include <time.h>

#if defined(__linux__)
#include <sys/random.h>
#endif
#if defined(__APPLE__) || defined(__OpenBSD__) || defined(__FreeBSD__)
#include <stdlib.h>	/* arc4random_buf */
#endif

void mcpkg_sip_seed(uint64_t *k0, uint64_t *k1)
{
	uint64_t s0 = 0, s1 = 0;
	int ok = 0;

	if (!k0 || !k1)
		return;

#if defined(__linux__)
	if (!ok) {
		ssize_t n = getrandom(&s0, sizeof(s0), 0);
		if (n == (ssize_t)sizeof(s0)) {
			n = getrandom(&s1, sizeof(s1), 0);
			if (n == (ssize_t)sizeof(s1))
				ok = 1;
		}
	}
#endif
#if defined(__APPLE__) || defined(__OpenBSD__) || defined(__FreeBSD__)
	if (!ok) {
		arc4random_buf(&s0, sizeof(s0));
		arc4random_buf(&s1, sizeof(s1));
		ok = 1;
	}
#endif
	if (!ok) {
		uint64_t t = (uint64_t)time(NULL);
		uintptr_t p = (uintptr_t)k0;
		s0 = (t ^ (uint64_t)p) * 0x9e3779b97f4a7c15ULL;
		s1 = (t + (uint64_t)(p >> 3)) ^ 0xbf58476d1ce4e5b9ULL;
	}

	*k0 = s0;
	*k1 = s1;
}
