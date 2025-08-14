/* SPDX-License-Identifier: MIT */
#include "mcpkg_thread_future.h"

#include <stdlib.h>

struct McPkgThreadFutureWatch {
	mcpkg_thread_future_watch_fn	fn;
	void				*user;
	struct McPkgThreadFutureWatch	*next;
};

struct McPkgThreadFuture {
	struct McPkgMutex       *lock;
	struct McPkgCond        *cv;
	int                     done;		/* 0/1 */
	int                     err;
	void                   *result;

	struct McPkgThreadFutureWatch *watchers;
};

static void
mcpkg_thread_future_invoke_watchers(struct McPkgThreadFutureWatch *w,
                                    void *result, int err)
{
	while (w) {
		struct McPkgThreadFutureWatch *next = w->next;
		mcpkg_thread_future_watch_fn fn = w->fn;
		void *user = w->user;
		free(w);
		if (fn)
			fn(user, result, err);
		w = next;
	}
}

struct McPkgThreadFuture *mcpkg_thread_future_new(void)
{
	struct McPkgThreadFuture *f = (struct McPkgThreadFuture *)calloc(1, sizeof(*f));
	if (!f)
		return NULL;

	f->lock = mcpkg_mutex_new();
	f->cv   = mcpkg_cond_new();
	if (!f->lock || !f->cv) {
		if (f->cv)
			mcpkg_cond_free(f->cv);
		if (f->lock)
			mcpkg_mutex_free(f->lock);
		free(f);
		return NULL;
	}
	return f;
}

void mcpkg_thread_future_free(struct McPkgThreadFuture *f)
{
	struct McPkgThreadFutureWatch *w, *next;

	if (!f)
		return;

	/* drop any remaining watchers without invoking */
	w = f->watchers;
	while (w) {
		next = w->next;
		free(w);
		w = next;
	}

	mcpkg_cond_free(f->cv);
	mcpkg_mutex_free(f->lock);
	free(f);
}

int mcpkg_thread_future_set(struct McPkgThreadFuture *f, void *result, int err)
{
	struct McPkgThreadFutureWatch *to_invoke;

	if (!f)
		return MCPKG_THREAD_E_INVAL;

	mcpkg_mutex_lock(f->lock);

	if (f->done) {
		mcpkg_mutex_unlock(f->lock);
		return MCPKG_THREAD_E_AGAIN;
	}

	f->done   = 1;
	f->result = result;
	f->err    = err;

	to_invoke   = f->watchers;
	f->watchers = NULL;

	mcpkg_cond_broadcast(f->cv);
	mcpkg_mutex_unlock(f->lock);

	mcpkg_thread_future_invoke_watchers(to_invoke, result, err);
	return MCPKG_THREAD_NO_ERROR;
}

int mcpkg_thread_future_wait(struct McPkgThreadFuture *f,
                             unsigned long timeout_ms,
                             void **out_result,
                             int *out_err)
{
	if (!f)
		return MCPKG_THREAD_E_INVAL;

	/* compute deadline before taking lock */
	uint64_t deadline = 0;
	if (timeout_ms != 0)
		deadline = mcpkg_thread_time_ms() + (uint64_t)timeout_ms;

	mcpkg_mutex_lock(f->lock);

	if (timeout_ms == 0) {
		/* infinite wait: classic while-loop */
		while (!f->done)
			mcpkg_cond_wait(f->cv, f->lock);
	} else {
		/* timed wait: loop until done or deadline reached */
		while (!f->done) {
			uint64_t now = mcpkg_thread_time_ms();
			if (now >= deadline) {
				mcpkg_mutex_unlock(f->lock);
				return MCPKG_THREAD_E_TIMEOUT;
			}
			unsigned long remain = (unsigned long)(deadline - now);
			int rc = mcpkg_cond_timedwait(f->cv, f->lock, remain);
			if (rc == MCPKG_THREAD_E_TIMEOUT) {
				/* re-check in case of racy set; otherwise loop until deadline trips */
				continue;
			}
			/* rc==0 or spurious wake → loop predicate */
		}
	}

	if (out_result) *out_result = f->result;
	if (out_err)    *out_err    = f->err;

	mcpkg_mutex_unlock(f->lock);
	return MCPKG_THREAD_NO_ERROR;
}


int mcpkg_thread_future_poll(struct McPkgThreadFuture *f,
                             void **out_result,
                             int *out_err)
{
	int done;

	if (!f)
		return 0;

	mcpkg_mutex_lock(f->lock);
	done = f->done;
	if (done) {
		if (out_result)
			*out_result = f->result;
		if (out_err)
			*out_err = f->err;
	}
	mcpkg_mutex_unlock(f->lock);
	return done ? 1 : 0;
}

int mcpkg_thread_future_watch(struct McPkgThreadFuture *f,
                              mcpkg_thread_future_watch_fn fn,
                              void *user)
{
	if (!f || !fn)
		return MCPKG_THREAD_E_INVAL;

	/* fast path: already done */
	mcpkg_mutex_lock(f->lock);
	if (f->done) {
		void *res = f->result;
		int err = f->err;
		mcpkg_mutex_unlock(f->lock);
		fn(user, res, err);
		return MCPKG_THREAD_NO_ERROR;
	}

	/* queue watcher */
	{
		struct McPkgThreadFutureWatch *w =
		        (struct McPkgThreadFutureWatch *)malloc(sizeof(*w));
		if (!w) {
			mcpkg_mutex_unlock(f->lock);
			return MCPKG_THREAD_E_NOMEM;
		}
		w->fn = fn;
		w->user = user;
		w->next = f->watchers;
		f->watchers = w;
	}

	mcpkg_mutex_unlock(f->lock);
	return MCPKG_THREAD_NO_ERROR;
}
