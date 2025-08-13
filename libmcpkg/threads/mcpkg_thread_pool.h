/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_THREAD_POOL_H
#define MCPKG_THREAD_POOL_H

#include "mcpkg_export.h"
#include "mcpkg_thread.h"
#include "mcpkg_thread_util.h"
#include "mcpkg_thread_future.h"

MCPKG_BEGIN_DECLS

struct McPkgThreadPool;		/* opaque */

typedef int (*mcpkg_thread_task_fn)(void *arg);

/* Config */
struct McPkgThreadPoolCfg {
	unsigned	threads;	/* 1..64 */
	unsigned	q_capacity;	/* >= threads */
};

/* lifecycle */
MCPKG_API int mcpkg_thread_pool_new(const struct McPkgThreadPoolCfg *cfg,
                                    struct McPkgThreadPool **out);
MCPKG_API int mcpkg_thread_pool_shutdown(struct McPkgThreadPool
                *p); /* graceful: drain */
MCPKG_API void mcpkg_thread_pool_free(struct McPkgThreadPool *p);

/* submit */
MCPKG_API int mcpkg_thread_pool_submit(struct McPkgThreadPool *p,
                                       mcpkg_thread_task_fn fn, void *arg); /* blocks if full */
MCPKG_API int mcpkg_thread_pool_try_submit(struct McPkgThreadPool *p,
                mcpkg_thread_task_fn fn, void *arg); /* E_AGAIN if full */

/* wait for queue empty and no active workers */
MCPKG_API int mcpkg_thread_pool_drain(struct McPkgThreadPool *p);

/* stats (approx, under lock when read) */
MCPKG_API unsigned mcpkg_thread_pool_queued(struct McPkgThreadPool *p);
MCPKG_API unsigned mcpkg_thread_pool_active(struct McPkgThreadPool *p);

/* convenience: run a callable and get a future */
typedef int (*mcpkg_thread_call_fn)(void *arg, void **out_result, int *out_err);
/* returns 0 and sets *out_f on success */
MCPKG_API int mcpkg_thread_pool_call_future(struct McPkgThreadPool *p,
                mcpkg_thread_call_fn call, void *arg,
                struct McPkgThreadFuture **out_f);

MCPKG_END_DECLS
#endif /* MCPKG_THREAD_POOL_H */
