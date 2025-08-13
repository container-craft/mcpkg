#include "mcpkg_thread_pool.h"
#include "mcpkg_thread_promise.h"

#include <stdlib.h>

struct McPkgThreadTask {
	mcpkg_thread_task_fn	fn;
	void			*arg;
};

struct McPkgThreadPool {
	struct McPkgThread	**workers;
	unsigned		threads;

	struct McPkgMutex	*lock;
	struct McPkgCond	*cv_not_empty;
	struct McPkgCond	*cv_not_full;
	struct McPkgCond	*cv_drained;

	struct McPkgThreadTask	*q;
	unsigned		cap;
	unsigned		head;		/* pop */
	unsigned		tail;		/* push */
	unsigned		len;

	unsigned		active;		/* workers running a task */
	int			shutting_down;
	int			joined;
};

static int worker_main(void *arg)
{
	struct McPkgThreadPool *p = (struct McPkgThreadPool *)arg;

	for (;;) {
		struct McPkgThreadTask t = {0};

		mcpkg_mutex_lock(p->lock);

		while (p->len == 0 && !p->shutting_down)
			mcpkg_cond_wait(p->cv_not_empty, p->lock);

		if (p->len == 0 && p->shutting_down) {
			mcpkg_mutex_unlock(p->lock);
			break;
		}

		t = p->q[p->head];
		p->head = (p->head + 1U) % p->cap;
		p->len--;
		p->active++;
		mcpkg_cond_signal(p->cv_not_full);
		mcpkg_mutex_unlock(p->lock);

		if (t.fn)
			(void)t.fn(t.arg);

		mcpkg_mutex_lock(p->lock);
		p->active--;
		if (p->len == 0 && p->active == 0)
			mcpkg_cond_broadcast(p->cv_drained);
		mcpkg_mutex_unlock(p->lock);
	}

	return 0;
}

int mcpkg_thread_pool_new(const struct McPkgThreadPoolCfg *cfg,
                          struct McPkgThreadPool **out)
{
	struct McPkgThreadPool *p;
	unsigned i;

	if (!cfg || !out || cfg->threads == 0 || cfg->q_capacity == 0)
		return MCPKG_THREAD_E_INVAL;

	p = (struct McPkgThreadPool *)calloc(1, sizeof(*p));
	if (!p)
		return MCPKG_THREAD_E_NOMEM;

	p->cap = cfg->q_capacity;
	p->q = (struct McPkgThreadTask *)calloc(p->cap, sizeof(*p->q));
	p->lock = mcpkg_mutex_new();
	p->cv_not_empty = mcpkg_cond_new();
	p->cv_not_full = mcpkg_cond_new();
	p->cv_drained = mcpkg_cond_new();
	if (!p->q || !p->lock || !p->cv_not_empty || !p->cv_not_full
	    || !p->cv_drained) {
		mcpkg_thread_pool_free(p);
		return MCPKG_THREAD_E_NOMEM;
	}

	p->threads = cfg->threads;
	p->workers = (struct McPkgThread **)calloc(p->threads, sizeof(*p->workers));
	if (!p->workers) {
		mcpkg_thread_pool_free(p);
		return MCPKG_THREAD_E_NOMEM;
	}

	for (i = 0; i < p->threads; i++) {
		p->workers[i] = mcpkg_thread_create(worker_main, p);
		if (!p->workers[i]) {
			/* best-effort shutdown */
			p->shutting_down = 1;
			mcpkg_cond_broadcast(p->cv_not_empty);
			while (i-- > 0)
				(void)mcpkg_thread_join(p->workers[i]);
			mcpkg_thread_pool_free(p);
			return MCPKG_THREAD_E_SYS;
		}
	}

	*out = p;
	return MCPKG_THREAD_NO_ERROR;
}

static void pool_join_workers(struct McPkgThreadPool *p)
{
	unsigned i;

	if (p->joined)
		return;
	p->joined = 1;

	for (i = 0; i < p->threads; i++) {
		if (p->workers[i])
			(void)mcpkg_thread_join(p->workers[i]);
	}
}

int mcpkg_thread_pool_shutdown(struct McPkgThreadPool *p)
{
	if (!p)
		return MCPKG_THREAD_E_INVAL;

	mcpkg_mutex_lock(p->lock);
	p->shutting_down = 1;
	mcpkg_cond_broadcast(p->cv_not_empty);
	while (p->len > 0 || p->active > 0)
		mcpkg_cond_wait(p->cv_drained, p->lock);
	mcpkg_mutex_unlock(p->lock);

	pool_join_workers(p);
	return MCPKG_THREAD_NO_ERROR;
}

void mcpkg_thread_pool_free(struct McPkgThreadPool *p)
{
	if (!p)
		return;

	if (!p->shutting_down) {
		(void)mcpkg_thread_pool_shutdown(p);
	}

	if (p->workers)
		free(p->workers);
	if (p->q)
		free(p->q);

	if (p->cv_drained)
		mcpkg_cond_free(p->cv_drained);
	if (p->cv_not_full)
		mcpkg_cond_free(p->cv_not_full);
	if (p->cv_not_empty)
		mcpkg_cond_free(p->cv_not_empty);
	if (p->lock)
		mcpkg_mutex_free(p->lock);

	free(p);
}

int mcpkg_thread_pool_submit(struct McPkgThreadPool *p,
                             mcpkg_thread_task_fn fn, void *arg)
{
	if (!p || !fn)
		return MCPKG_THREAD_E_INVAL;

	mcpkg_mutex_lock(p->lock);

	if (p->shutting_down) {
		mcpkg_mutex_unlock(p->lock);
		return MCPKG_THREAD_E_AGAIN;
	}

	while (p->len == p->cap && !p->shutting_down)
		mcpkg_cond_wait(p->cv_not_full, p->lock);

	if (p->shutting_down) {
		mcpkg_mutex_unlock(p->lock);
		return MCPKG_THREAD_E_AGAIN;
	}

	p->q[p->tail].fn = fn;
	p->q[p->tail].arg = arg;
	p->tail = (p->tail + 1U) % p->cap;
	p->len++;

	mcpkg_cond_signal(p->cv_not_empty);
	mcpkg_mutex_unlock(p->lock);
	return MCPKG_THREAD_NO_ERROR;
}

int mcpkg_thread_pool_try_submit(struct McPkgThreadPool *p,
                                 mcpkg_thread_task_fn fn, void *arg)
{
	if (!p || !fn)
		return MCPKG_THREAD_E_INVAL;

	mcpkg_mutex_lock(p->lock);

	if (p->shutting_down || p->len == p->cap) {
		mcpkg_mutex_unlock(p->lock);
		return MCPKG_THREAD_E_AGAIN;
	}

	p->q[p->tail].fn = fn;
	p->q[p->tail].arg = arg;
	p->tail = (p->tail + 1U) % p->cap;
	p->len++;

	mcpkg_cond_signal(p->cv_not_empty);
	mcpkg_mutex_unlock(p->lock);
	return MCPKG_THREAD_NO_ERROR;
}

int mcpkg_thread_pool_drain(struct McPkgThreadPool *p)
{
	if (!p)
		return MCPKG_THREAD_E_INVAL;

	mcpkg_mutex_lock(p->lock);
	while (p->len > 0 || p->active > 0)
		mcpkg_cond_wait(p->cv_drained, p->lock);
	mcpkg_mutex_unlock(p->lock);
	return MCPKG_THREAD_NO_ERROR;
}

unsigned mcpkg_thread_pool_queued(struct McPkgThreadPool *p)
{
	unsigned v = 0;
	if (!p)
		return 0;
	mcpkg_mutex_lock(p->lock);
	v = p->len;
	mcpkg_mutex_unlock(p->lock);
	return v;
}

unsigned mcpkg_thread_pool_active(struct McPkgThreadPool *p)
{
	unsigned v = 0;
	if (!p)
		return 0;
	mcpkg_mutex_lock(p->lock);
	v = p->active;
	mcpkg_mutex_unlock(p->lock);
	return v;
}

/* ----- future convenience wrapper ----- */

struct CallCtx {
	mcpkg_thread_call_fn	call;
	void			*arg;
	struct McPkgThreadPromise *promise;
};

static int call_tramp(void *arg)
{
	struct CallCtx *cc = (struct CallCtx *)arg;
	void *res = NULL;
	int err = 0;

	if (cc->call)
		(void)cc->call(cc->arg, &res, &err);

	(void)mcpkg_thread_promise_set(cc->promise, res, err);
	mcpkg_thread_promise_free(cc->promise);
	free(cc);
	return 0;
}

int mcpkg_thread_pool_call_future(struct McPkgThreadPool *p,
                                  mcpkg_thread_call_fn call, void *arg,
                                  struct McPkgThreadFuture **out_f)
{
	struct McPkgThreadPromise *pr = NULL;
	struct McPkgThreadFuture *f = NULL;
	struct CallCtx *cc;

	if (!p || !out_f || !call)
		return MCPKG_THREAD_E_INVAL;

	if (mcpkg_thread_promise_new(&pr, &f) != MCPKG_THREAD_NO_ERROR)
		return MCPKG_THREAD_E_NOMEM;

	cc = (struct CallCtx *)calloc(1, sizeof(*cc));
	if (!cc) {
		mcpkg_thread_promise_free(pr);
		mcpkg_thread_future_free(f);
		return MCPKG_THREAD_E_NOMEM;
	}
	cc->call = call;
	cc->arg = arg;
	cc->promise = pr;

	*out_f = f;
	return mcpkg_thread_pool_submit(p, call_tramp, cc);
}
