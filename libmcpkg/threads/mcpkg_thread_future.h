/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_THREAD_FUTURE_H
#define MCPKG_THREAD_FUTURE_H

#include "mcpkg_export.h"
#include "mcpkg_thread.h"
#include "mcpkg_thread_util.h"

MCPKG_BEGIN_DECLS

struct McPkgThreadFuture;		/* opaque */
typedef void (*mcpkg_thread_future_watch_fn)(void *user, void *result, int err);

/* lifecycle */
MCPKG_API struct McPkgThreadFuture *mcpkg_thread_future_new(void);
MCPKG_API void mcpkg_thread_future_free(struct McPkgThreadFuture *f);

/* set once; err==0 for success */
MCPKG_API int mcpkg_thread_future_set(struct McPkgThreadFuture *f, void *result,
                                      int err);

/* wait for completion; timeout_ms==0 => infinite */
MCPKG_API int mcpkg_thread_future_wait(struct McPkgThreadFuture *f,
                                       unsigned long timeout_ms,
                                       void **out_result,
                                       int *out_err);

/* non-blocking peek; returns 1 if done, 0 if not */
MCPKG_API int mcpkg_thread_future_poll(struct McPkgThreadFuture *f,
                                       void **out_result,
                                       int *out_err);

/* register a watcher; invoked once on completion (or immediately if done) */
MCPKG_API int mcpkg_thread_future_watch(struct McPkgThreadFuture *f,
                                        mcpkg_thread_future_watch_fn fn,
                                        void *user);

MCPKG_END_DECLS
#endif /* MCPKG_THREAD_FUTURE_H */
