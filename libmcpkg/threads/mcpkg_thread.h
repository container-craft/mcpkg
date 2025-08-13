/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_THREAD_H
#define MCPKG_THREAD_H

#include <stdint.h>
#include <stddef.h>

#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS
/* Opaque types */
struct McPkgThread;
struct McPkgMutex;
struct McPkgCond;

typedef int (*mcpkg_thread_fn)(void *arg);

/* Threads */
MCPKG_API struct McPkgThread *mcpkg_thread_create(mcpkg_thread_fn fn,
                void *arg);
MCPKG_API int mcpkg_thread_join(struct McPkgThread *t);
MCPKG_API int mcpkg_thread_detach(struct McPkgThread *t);
MCPKG_API uint64_t mcpkg_thread_id(
        void);             /* stable-ish id for logging */
MCPKG_API int mcpkg_thread_set_name(const char
                                    *name);/* best effort; may be UNSUPPORTED */

/* Mutex */
MCPKG_API struct McPkgMutex *mcpkg_mutex_new(void);
MCPKG_API void mcpkg_mutex_free(struct McPkgMutex *m);
MCPKG_API void mcpkg_mutex_lock(struct McPkgMutex *m);
MCPKG_API void mcpkg_mutex_unlock(struct McPkgMutex *m);

/* Condvar */
MCPKG_API struct McPkgCond *mcpkg_cond_new(void);
MCPKG_API void mcpkg_cond_free(struct McPkgCond *c);
MCPKG_API void mcpkg_cond_wait(struct McPkgCond *c, struct McPkgMutex *m);
MCPKG_API int  mcpkg_cond_timedwait(struct McPkgCond *c, struct McPkgMutex *m,
                                    unsigned long timeout_ms);
MCPKG_API void mcpkg_cond_signal(struct McPkgCond *c);
MCPKG_API void mcpkg_cond_broadcast(struct McPkgCond *c);


MCPKG_END_DECLS
#endif
