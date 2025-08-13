#ifndef MCPKG_LIST_H
#define MCPKG_LIST_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "container/mcpkg_container_error.h"


MCPKG_BEGIN_DECLS

/*
 * McPkgList: dynamic array of fixed-size elements, by-value storage.
 * Not thread-safe; callers must synchronize.
 */

typedef struct McPkgList McPkgList;

/* Optional per-element hooks; pass NULL for memcpy/no-op/memcmp. */
typedef struct {
	void (*copy)(void *dst, const void *src, void *ctx);
	void (*dtor)(void *elem, void *ctx);
	int (*equals)(const void *a, const void *b, void *ctx);
	void *ctx;
} McPkgListOps;

/* Create a list for elements of elem_size; caps default or as provided. */
MCPKG_API McPkgList *mcpkg_list_new(size_t elem_size,
                                    const McPkgListOps *ops_or_null,
                                    size_t max_elements /*0=default*/,
                                    unsigned long long max_bytes /*0=default*/);

/* Free the list; calls dtor on remaining elements if provided. */
MCPKG_API void mcpkg_list_free(McPkgList *lst);

/* Resize to new_size (grow zero-fills; shrink calls dtor on truncated tail). */
MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_list_resize(McPkgList *lst, size_t new_size);

/* Ensure capacity is at least min_capacity (no size change). */
MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_list_reserve_at_least(McPkgList *lst, size_t min_capacity);

/* Current logical length and allocated capacity (in elements). */
MCPKG_API size_t mcpkg_list_size(const McPkgList *lst);
MCPKG_API size_t mcpkg_list_capacity(const McPkgList *lst);

/* Remove all elements (size→0), invoking dtor per element if set. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_list_remove_all(McPkgList *lst);

/* Remove element at index, shifting tail left by one. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_list_remove_at(McPkgList *lst,
                size_t index);

/* Append one element by value from *elem. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_list_push(McPkgList *lst,
                const void *elem);

/* Pop last element; if out!=NULL copy it before removal. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_list_pop(McPkgList *lst, void *out);

/* Insert one element at index (shifts right). */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_list_add(McPkgList *lst, size_t index,
                const void *elem);

/* Copy element at index to *out. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_list_at(const McPkgList *lst,
                size_t index, void *out);

/* Borrowed pointer to element at index; invalidated by reallocation. */
MCPKG_API void *mcpkg_list_at_ptr(McPkgList *lst, size_t index);
MCPKG_API const void *mcpkg_list_at_cptr(const McPkgList *lst, size_t index);

/* Copy first/last element to *out. */
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_list_first(const McPkgList *lst,
                void *out);
MCPKG_API MCPKG_CONTAINER_ERROR mcpkg_list_last(const McPkgList *lst,
                void *out);

/* Linear search; uses equals if provided, else memcmp; returns index or -1. */
MCPKG_API int mcpkg_list_index_of(const McPkgList *lst, const void *needle);

/* Inspect or update per-instance limits (0 leaves a field unchanged). */
MCPKG_API void mcpkg_list_get_limits(const McPkgList *lst,
                                     size_t *max_elements,
                                     unsigned long long *max_bytes);

MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_list_set_limits(McPkgList *lst, size_t max_elements,
                      unsigned long long max_bytes);

MCPKG_END_DECLS
#endif /* MCPKG_LIST_H */
