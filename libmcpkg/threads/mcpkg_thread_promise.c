/* SPDX-License-Identifier: MIT */
#include "mcpkg_thread_promise.h"

#include <stdlib.h>

struct McPkgThreadPromise {
	struct McPkgThreadFuture *f;
};

int mcpkg_thread_promise_new(struct McPkgThreadPromise **out_p,
                             struct McPkgThreadFuture **out_f)
{
	struct McPkgThreadPromise *p;
	struct McPkgThreadFuture *f;

	if (!out_p || !out_f)
		return MCPKG_THREAD_E_INVAL;

	f = mcpkg_thread_future_new();
	if (!f)
		return MCPKG_THREAD_E_NOMEM;

	p = (struct McPkgThreadPromise *)calloc(1, sizeof(*p));
	if (!p) {
		mcpkg_thread_future_free(f);
		return MCPKG_THREAD_E_NOMEM;
	}

	p->f = f;
	*out_p = p;
	*out_f = f;
	return MCPKG_THREAD_NO_ERROR;
}

void mcpkg_thread_promise_free(struct McPkgThreadPromise *p)
{
	if (!p)
		return;
	/* does NOT free p->f; consumer owns the future */
	free(p);
}

struct McPkgThreadFuture *mcpkg_thread_promise_future(struct McPkgThreadPromise
                *p)
{
	return p ? p->f : NULL;
}

int mcpkg_thread_promise_set(struct McPkgThreadPromise *p, void *result,
                             int err)
{
	if (!p || !p->f)
		return MCPKG_THREAD_E_INVAL;
	return mcpkg_thread_future_set(p->f, result, err);
}
