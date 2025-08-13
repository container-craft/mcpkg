#include "mcpkg_str_list.h"
#include "mcpkg_str_list_p.h"
#include <limits.h>

McPkgStringList *mcpkg_stringlist_new(size_t max_elements,
                                      unsigned long long max_bytes)
{
	McPkgStringList *sl;

	sl = calloc(1, sizeof(*sl));
	if (!sl)
		return NULL;

	sl->lst = strlist_make_base(max_elements, max_bytes);
	if (!sl->lst) {
		free(sl);
		return NULL;
	}

	return sl;
}

void mcpkg_stringlist_free(McPkgStringList *sl)
{
	if (!sl)
		return;

	if (sl->lst)
		mcpkg_list_free(sl->lst);

	free(sl);
}

MCPKG_CONTAINER_ERROR
mcpkg_stringlist_resize(McPkgStringList *sl, size_t new_size)
{
	size_t old_len, i;
	MCPKG_CONTAINER_ERROR ret;

	if (!sl)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	old_len = mcpkg_list_size(sl->lst);
	if (new_size <= old_len)
		return mcpkg_list_resize(sl->lst, new_size);

	ret = mcpkg_list_resize(sl->lst, new_size);
	if (ret != MCPKG_CONTAINER_OK)
		return ret;

	for (i = old_len; i < new_size; i++) {
		char **cell = (char **)mcpkg_list_at_ptr(sl->lst, i);
		char *dup;

		if (!cell)
			goto fail_shrink;

		dup = strdup("");
		if (!dup)
			goto fail_shrink;

		*cell = dup;
	}

	return MCPKG_CONTAINER_OK;

fail_shrink:
	(void)mcpkg_list_resize(sl->lst, old_len);
	return MCPKG_CONTAINER_ERR_NO_MEM;
}

size_t mcpkg_stringlist_size(const McPkgStringList *sl)
{
	return sl ? mcpkg_list_size(sl->lst) : 0;
}

MCPKG_CONTAINER_ERROR
mcpkg_stringlist_remove_all(McPkgStringList *sl)
{
	if (!sl)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	return mcpkg_list_remove_all(sl->lst);
}

MCPKG_CONTAINER_ERROR
mcpkg_stringlist_remove_at(McPkgStringList *sl, size_t index)
{
	if (!sl)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	return mcpkg_list_remove_at(sl->lst, index);
}

MCPKG_CONTAINER_ERROR
mcpkg_stringlist_push(McPkgStringList *sl, const char *s)
{
	char *dup;
	MCPKG_CONTAINER_ERROR ret;

	if (!sl)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	dup = strdup(s ? s : "");
	if (!dup)
		return MCPKG_CONTAINER_ERR_NO_MEM;

	ret = mcpkg_list_push(sl->lst, &dup);
	if (ret != MCPKG_CONTAINER_OK) {
		free(dup);
		return ret;
	}
	return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR
mcpkg_stringlist_pop(McPkgStringList *sl, char **out)
{
	size_t n, idx;
	char **cell;

	if (!sl)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	n = mcpkg_list_size(sl->lst);
	if (!n)
		return MCPKG_CONTAINER_ERR_RANGE;

	idx = n - 1;
	cell = (char **)mcpkg_list_at_ptr(sl->lst, idx);
	if (!cell)
		return MCPKG_CONTAINER_ERR_RANGE;

	if (out) {
		*out = *cell;
		*cell = NULL;
		return mcpkg_list_pop(sl->lst, NULL);
	}
	return mcpkg_list_pop(sl->lst, NULL);
}

MCPKG_CONTAINER_ERROR
mcpkg_stringlist_add(McPkgStringList *sl, size_t index, const char *s)
{
	char *dup;
	MCPKG_CONTAINER_ERROR ret;

	if (!sl)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	dup = strdup(s ? s : "");
	if (!dup)
		return MCPKG_CONTAINER_ERR_NO_MEM;

	ret = mcpkg_list_add(sl->lst, index, &dup);
	if (ret != MCPKG_CONTAINER_OK) {
		free(dup);
		return ret;
	}
	return MCPKG_CONTAINER_OK;
}

const char *mcpkg_stringlist_at(const McPkgStringList *sl, size_t index)
{
	if (!sl)
		return NULL;
	return sl_at_borrow(sl, index);
}

const char *mcpkg_stringlist_first(const McPkgStringList *sl)
{
	if (!sl)
		return NULL;
	return mcpkg_stringlist_size(sl) ? sl_at_borrow(sl, 0) : NULL;
}

const char *mcpkg_stringlist_last(const McPkgStringList *sl)
{
	size_t n;

	if (!sl)
		return NULL;

	n = mcpkg_stringlist_size(sl);
	return n ? sl_at_borrow(sl, n - 1) : NULL;
}

int mcpkg_stringlist_index_of(const McPkgStringList *sl, const char *s)
{
	size_t i, n;
	const char *needle = s ? s : "";

	if (!sl)
		return -1;

	n = mcpkg_stringlist_size(sl);
	for (i = 0; i < n; i++) {
		const char *v = sl_at_borrow(sl, i);

		if (v && strcmp(v, needle) == 0)
			return (i > (size_t)INT_MAX) ? -1 : (int)i;
		if (!v && *needle == '\0')
			return (i > (size_t)INT_MAX) ? -1 : (int)i;
	}
	return -1;
}

void mcpkg_stringlist_get_limits(const McPkgStringList *sl,
                                 size_t *max_elements,
                                 unsigned long long *max_bytes)
{
	if (!sl) {
		if (max_elements)
			*max_elements = 0;
		if (max_bytes)
			*max_bytes = 0;
		return;
	}
	mcpkg_list_get_limits(sl->lst, max_elements, max_bytes);
}

MCPKG_CONTAINER_ERROR
mcpkg_stringlist_set_limits(McPkgStringList *sl, size_t max_elements,
                            unsigned long long max_bytes)
{
	if (!sl)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	return mcpkg_list_set_limits(sl->lst, max_elements, max_bytes);
}
