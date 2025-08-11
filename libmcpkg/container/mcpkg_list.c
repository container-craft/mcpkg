#include "mcpkg_list.h"
#include "mcpkg_str_list_p.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "container/mcpkg_container_util.h"

/*
 * McPkgList: dynamic array of fixed-size elements, by-value storage.
 * Not thread-safe; callers must synchronize.
 */

struct McPkgList {
    unsigned char	*data;              /* cap * elem_size bytes */
    size_t          len;                /* elements in use */
    size_t          cap;                /* capacity (elements) */
    size_t          elem_size;          /* bytes per element */
    McPkgListOps    ops;                /* copy/dtor/equals/ctx */
    size_t          max_elements;       /* hard cap (count) */
    unsigned long long	max_bytes;     /* hard cap (bytes) */
};

/* ----- internal helpers ----- */

static MCPKG_CONTAINER_ERROR grow_to(McPkgList *lst, size_t new_cap)
{
	size_t eff_max, bytes;
	void *p;

	if (!lst)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	if (new_cap <= lst->cap)
		return MCPKG_CONTAINER_OK;

	eff_max = mcpkg_effective_max_elements(lst->max_elements,
					       lst->max_bytes,
					       lst->elem_size);
	if (!eff_max)
		return MCPKG_CONTAINER_ERR_LIMIT;

	if (new_cap > eff_max)
		new_cap = eff_max;

	if (new_cap <= lst->cap)
		return MCPKG_CONTAINER_OK;

	if (mcpkg_mul_overflow_size(new_cap, lst->elem_size, &bytes))
		return MCPKG_CONTAINER_ERR_OVERFLOW;

	if ((unsigned long long)bytes > lst->max_bytes)
		return MCPKG_CONTAINER_ERR_LIMIT;

	p = realloc(lst->data, bytes);
	if (!p)
		return MCPKG_CONTAINER_ERR_NO_MEM;

	lst->data = (unsigned char *)p;
	lst->cap = new_cap;
	return MCPKG_CONTAINER_OK;
}

static MCPKG_CONTAINER_ERROR ensure_cap_for(McPkgList *lst, size_t want_len)
{
    size_t new_cap, eff_max; //3

	if (!lst)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	if (want_len > lst->max_elements)
		return MCPKG_CONTAINER_ERR_LIMIT;

	if (want_len <= lst->cap)
		return MCPKG_CONTAINER_OK;

	new_cap = lst->cap ? (lst->cap << 1) : 8;
	new_cap = new_cap < want_len ? want_len : new_cap;

	eff_max = mcpkg_effective_max_elements(lst->max_elements,
					       lst->max_bytes,
					       lst->elem_size);
	if (!eff_max)
		return MCPKG_CONTAINER_ERR_LIMIT;

	if (new_cap > eff_max)
		new_cap = eff_max;

	return grow_to(lst, new_cap);
}

/* ----- public API ----- */

McPkgList *mcpkg_list_new(size_t elem_size,
                          const McPkgListOps *ops_or_null,
                          size_t max_elements,
                          unsigned long long max_bytes)
{
	McPkgList *lst;
	size_t eff;

	if (!elem_size)
		return NULL;

	lst = calloc(1, sizeof(*lst));
	if (!lst)
		return NULL;

	lst->elem_size = elem_size;

	if (ops_or_null)
		lst->ops = *ops_or_null;
	else
		memset(&lst->ops, 0, sizeof(lst->ops));

	lst->max_elements = max_elements ? max_elements
					 : MCPKG_CONTAINER_MAX_ELEMENTS;
	lst->max_bytes = max_bytes ? max_bytes : MCPKG_CONTAINER_MAX_BYTES;

	eff = mcpkg_effective_max_elements(lst->max_elements,
					   lst->max_bytes,
					   lst->elem_size);
	if (!eff) {
		free(lst);
		return NULL;
	}

	return lst;
}

void mcpkg_list_free(McPkgList *lst)
{
	size_t i;

	if (!lst)
		return;

	if (lst->ops.dtor && lst->data) {
		for (i = 0; i < lst->len; i++) {
			void *e = lst->data + i * lst->elem_size;
			lst->ops.dtor(e, lst->ops.ctx);
		}
	}

#ifdef MCPKG_LIST_MEMZERO_ON_FREE
	if (lst->data && lst->cap) {
		size_t bytes = lst->cap * lst->elem_size;
		memset(lst->data, 0, bytes);
	}
#endif
	free(lst->data);
	free(lst);
}

MCPKG_CONTAINER_ERROR mcpkg_list_resize(McPkgList *lst, size_t new_size)
{
	size_t old_len, add, bytes;
	size_t i;
	MCPKG_CONTAINER_ERROR ret;

	if (!lst)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	if (new_size == lst->len)
		return MCPKG_CONTAINER_OK;

	/* shrink */
	if (new_size < lst->len) {
		if (lst->ops.dtor) {
			for (i = new_size; i < lst->len; i++) {
				void *e = lst->data + i * lst->elem_size;
				lst->ops.dtor(e, lst->ops.ctx);
			}
		}
		lst->len = new_size;
		return MCPKG_CONTAINER_OK;
	}

	/* grow */
	ret = ensure_cap_for(lst, new_size);
	if (ret != MCPKG_CONTAINER_OK)
		return ret;

	old_len = lst->len;
	add = new_size - old_len;

	if (mcpkg_mul_overflow_size(add, lst->elem_size, &bytes))
		return MCPKG_CONTAINER_ERR_OVERFLOW;

	memset(lst->data + old_len * lst->elem_size, 0, bytes);
	lst->len = new_size;
	return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_list_reserve_at_least(McPkgList *lst,
						  size_t min_capacity)
{
	if (!lst)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	if (min_capacity <= lst->cap)
		return MCPKG_CONTAINER_OK;
	return grow_to(lst, min_capacity);
}

size_t mcpkg_list_size(const McPkgList *lst)
{
	return lst ? lst->len : 0;
}

size_t mcpkg_list_capacity(const McPkgList *lst)
{
	return lst ? lst->cap : 0;
}

MCPKG_CONTAINER_ERROR mcpkg_list_remove_all(McPkgList *lst)
{
	size_t i;

	if (!lst)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	if (lst->ops.dtor) {
		for (i = 0; i < lst->len; i++) {
			void *e = lst->data + i * lst->elem_size;
			lst->ops.dtor(e, lst->ops.ctx);
		}
	}
	lst->len = 0;
	return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_list_remove_at(McPkgList *lst, size_t index)
{
	size_t tail_elems, bytes;

	if (!lst)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	if (index >= lst->len)
		return MCPKG_CONTAINER_ERR_RANGE;

	if (lst->ops.dtor) {
		void *e = lst->data + index * lst->elem_size;
		lst->ops.dtor(e, lst->ops.ctx);
	}

	if (index + 1 < lst->len) {
		tail_elems = lst->len - index - 1;
		if (mcpkg_mul_overflow_size(tail_elems, lst->elem_size, &bytes))
			return MCPKG_CONTAINER_ERR_OVERFLOW;

		memmove(lst->data + index * lst->elem_size,
			lst->data + (index + 1) * lst->elem_size,
			bytes);
	}

	lst->len--;
	return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_list_push(McPkgList *lst, const void *elem)
{
	MCPKG_CONTAINER_ERROR ret;

	if (!lst || !elem)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

    ret = ensure_cap_for(lst, lst->len + 1); //2
	if (ret != MCPKG_CONTAINER_OK)
		return ret;

	if (lst->ops.copy) {
		lst->ops.copy(lst->data + lst->len * lst->elem_size, elem,
			      lst->ops.ctx);
	} else {
		memcpy(lst->data + lst->len * lst->elem_size, elem,
		       lst->elem_size);
	}

	lst->len++;
	return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_list_pop(McPkgList *lst, void *out)
{
	size_t idx;
	void *src;

	if (!lst)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	if (!lst->len)
		return MCPKG_CONTAINER_ERR_RANGE;

	idx = lst->len - 1;
	src = lst->data + idx * lst->elem_size;

	if (out)
		memcpy(out, src, lst->elem_size);

	if (lst->ops.dtor)
		lst->ops.dtor(src, lst->ops.ctx);

	lst->len--;
	return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_list_add(McPkgList *lst, size_t index,
				     const void *elem)
{
	size_t move_elems, bytes;
	MCPKG_CONTAINER_ERROR ret;

	if (!lst || !elem)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	if (index > lst->len)
		return MCPKG_CONTAINER_ERR_RANGE;

	ret = ensure_cap_for(lst, lst->len + 1);
	if (ret != MCPKG_CONTAINER_OK)
		return ret;

	if (index < lst->len) {
		move_elems = lst->len - index;
		if (mcpkg_mul_overflow_size(move_elems, lst->elem_size, &bytes))
			return MCPKG_CONTAINER_ERR_OVERFLOW;

		memmove(lst->data + (index + 1) * lst->elem_size,
			lst->data + index * lst->elem_size,
			bytes);
	}

	if (lst->ops.copy) {
		lst->ops.copy(lst->data + index * lst->elem_size, elem,
			      lst->ops.ctx);
	} else {
		memcpy(lst->data + index * lst->elem_size, elem,
		       lst->elem_size);
	}

	lst->len++;
	return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_list_at(const McPkgList *lst, size_t index,
				    void *out)
{
	if (!lst || !out)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	if (index >= lst->len)
		return MCPKG_CONTAINER_ERR_RANGE;

	memcpy(out, lst->data + index * lst->elem_size, lst->elem_size);
	return MCPKG_CONTAINER_OK;
}

void *mcpkg_list_at_ptr(McPkgList *lst, size_t index)
{
	if (!lst || index >= lst->len)
		return NULL;
	return lst->data + index * lst->elem_size;
}

const void *mcpkg_list_at_cptr(const McPkgList *lst, size_t index)
{
	if (!lst || index >= lst->len)
		return NULL;
	return lst->data + index * lst->elem_size;
}

MCPKG_CONTAINER_ERROR mcpkg_list_first(const McPkgList *lst, void *out)
{
	if (!lst || !out)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	if (!lst->len)
		return MCPKG_CONTAINER_ERR_RANGE;

	memcpy(out, lst->data, lst->elem_size);
	return MCPKG_CONTAINER_OK;
}

MCPKG_CONTAINER_ERROR mcpkg_list_last(const McPkgList *lst, void *out)
{
	if (!lst || !out)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;
	if (!lst->len)
		return MCPKG_CONTAINER_ERR_RANGE;

	memcpy(out, lst->data + (lst->len - 1) * lst->elem_size,
	       lst->elem_size);
	return MCPKG_CONTAINER_OK;
}

int mcpkg_list_index_of(const McPkgList *lst, const void *needle)
{
	size_t i;

	if (!lst || !needle)
		return -1;
	if (!lst->len)
		return -1;

	if (lst->ops.equals) {
		for (i = 0; i < lst->len; i++) {
			const void *e = lst->data + i * lst->elem_size;
			if (lst->ops.equals(e, needle, lst->ops.ctx) == 0)
				return (i > (size_t)INT_MAX) ? -1 : (int)i;
		}
	} else {
		for (i = 0; i < lst->len; i++) {
			const void *e = lst->data + i * lst->elem_size;
			if (!memcmp(e, needle, lst->elem_size))
				return (i > (size_t)INT_MAX) ? -1 : (int)i;
		}
	}

	return -1;
}

void mcpkg_list_get_limits(const McPkgList *lst, size_t *max_elements,
			   unsigned long long *max_bytes)
{
	if (!lst) {
		if (max_elements)
			*max_elements = 0;
		if (max_bytes)
			*max_bytes = 0;
		return;
	}

	if (max_elements)
		*max_elements = lst->max_elements;
	if (max_bytes)
		*max_bytes = lst->max_bytes;
}

MCPKG_CONTAINER_ERROR mcpkg_list_set_limits(McPkgList *lst,
					    size_t max_elements,
					    unsigned long long max_bytes)
{
	size_t new_max_elems, need_bytes;

	if (!lst)
		return MCPKG_CONTAINER_ERR_NULL_PARAM;

	new_max_elems = max_elements ? max_elements : lst->max_elements;
	max_bytes = max_bytes ? max_bytes : lst->max_bytes;

	if (lst->len > new_max_elems)
		return MCPKG_CONTAINER_ERR_LIMIT;

	if (mcpkg_mul_overflow_size(lst->cap, lst->elem_size, &need_bytes))
		return MCPKG_CONTAINER_ERR_OVERFLOW;

	if ((unsigned long long)need_bytes > max_bytes)
		return MCPKG_CONTAINER_ERR_LIMIT;

	if (!mcpkg_effective_max_elements(new_max_elems, max_bytes,
					  lst->elem_size))
		return MCPKG_CONTAINER_ERR_LIMIT;

	lst->max_elements = new_max_elems;
	lst->max_bytes = max_bytes;
	return MCPKG_CONTAINER_OK;
}
