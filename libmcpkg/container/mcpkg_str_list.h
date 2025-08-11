#ifndef MCPKG_STR_LIST_H
#define MCPKG_STR_LIST_H

#include <stddef.h>
#include "mcpkg_export.h"
#include "container/mcpkg_container_error.h"

MCPKG_BEGIN_DECLS

/*
 * McPkgStringList: owned list of char*.
 * push/add strdup the input; remove/clear/free release strings.
 * Not thread-safe; callers must synchronize.
 */

typedef struct McPkgStringList McPkgStringList;

/* Create an empty list; caps default or as provided. */
MCPKG_API McPkgStringList *mcpkg_stringlist_new(size_t max_elements /*0=def*/,
						unsigned long long max_bytes
						/*0=def*/);

/* Free the list and all owned strings. */
MCPKG_API void mcpkg_stringlist_free(McPkgStringList *sl);

/* Resize to new_size; growing fills with "" (owned empty strings). */
MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_stringlist_resize(McPkgStringList *sl, size_t new_size);

/* Current number of elements. */
MCPKG_API size_t mcpkg_stringlist_size(const McPkgStringList *sl);

/* Remove all elements (size→0; frees each string). */
MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_stringlist_remove_all(McPkgStringList *sl);

/* Remove element at index (frees), shifting tail left by one. */
MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_stringlist_remove_at(McPkgStringList *sl, size_t index);

/* Append a copy (strdup) of s. */
MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_stringlist_push(McPkgStringList *sl, const char *s);

/* Pop last; if out!=NULL, transfer ownership of the string, else free it. */
MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_stringlist_pop(McPkgStringList *sl, char **out /*nullable*/);

/* Insert a copy (strdup) of s at index (shifts right). */
MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_stringlist_add(McPkgStringList *sl, size_t index, const char *s);

/* Borrowed pointer to element at index; invalid after mutating reallocation. */
MCPKG_API const char *mcpkg_stringlist_at(const McPkgStringList *sl,
					  size_t index);

/* Borrowed pointer to first/last; invalid after mutating reallocation. */
MCPKG_API const char *mcpkg_stringlist_first(const McPkgStringList *sl);
MCPKG_API const char *mcpkg_stringlist_last(const McPkgStringList *sl);

/* Linear search using strcmp; returns index or -1. */
MCPKG_API int mcpkg_stringlist_index_of(const McPkgStringList *sl,
					const char *s);

/* Inspect or update per-instance limits (0 leaves a field unchanged). */
MCPKG_API void mcpkg_stringlist_get_limits(const McPkgStringList *sl,
					   size_t *max_elements,
					   unsigned long long *max_bytes);

MCPKG_API MCPKG_CONTAINER_ERROR
mcpkg_stringlist_set_limits(McPkgStringList *sl, size_t max_elements,
			    unsigned long long max_bytes);

MCPKG_END_DECLS
#endif /* MCPKG_STR_LIST_H */
