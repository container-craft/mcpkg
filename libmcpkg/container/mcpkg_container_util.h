#ifndef MCPKG_CONTAINER_UTIL_H
#define MCPKG_CONTAINER_UTIL_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "math/mcpkg_math.h"

MCPKG_BEGIN_DECLS

/*
 * Effective element cap given both a count cap and a byte cap.
 * Returns min(max_elements, max_bytes / elem_size); 0 if elem_size == 0.
 */
static inline size_t
mcpkg_effective_max_elements(size_t max_elements,
                             unsigned long long max_bytes,
                             size_t elem_size)
{
	if (!elem_size)
		return 0;
	return mcpkg_math_min_size(max_elements,
	                           (size_t)(max_bytes /
	                                    (unsigned long long)elem_size));
}

MCPKG_END_DECLS
#endif /* MCPKG_CONTAINER_UTIL_H */
