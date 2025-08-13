/* SPDX-License-Identifier: MIT */
#ifndef MCPKG_THREAD_POSIX_H
#define MCPKG_THREAD_POSIX_H

#include <stdint.h>
#include <pthread.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS

/* Opaque structs for this TU */
struct McPkgThread {
	pthread_t t;
	int joined;
	int detached;
};
struct McPkgMutex {
	pthread_mutex_t m;
};
struct McPkgCond {
	pthread_cond_t c;
};

MCPKG_API struct McPkgThread *mcpkg_thread_impl_create(int (*fn)(void *),
                void *arg);
MCPKG_API int mcpkg_thread_impl_join(struct McPkgThread *t);
MCPKG_API int mcpkg_thread_impl_detach(struct McPkgThread *t);
MCPKG_API uint64_t mcpkg_thread_impl_id(void);
MCPKG_API int mcpkg_thread_impl_set_name(const char *name);

struct McPkgMutex *mcpkg_mutex_impl_new(void);
MCPKG_API void mcpkg_mutex_impl_free(struct McPkgMutex *m);
MCPKG_API void mcpkg_mutex_impl_lock(struct McPkgMutex *m);
MCPKG_API void mcpkg_mutex_impl_unlock(struct McPkgMutex *m);

struct McPkgCond *mcpkg_cond_impl_new(void);
MCPKG_API void mcpkg_cond_impl_free(struct McPkgCond *c);
MCPKG_API void mcpkg_cond_impl_wait(struct McPkgCond *c, struct McPkgMutex *m);
MCPKG_API int  mcpkg_cond_impl_timedwait(struct McPkgCond *c,
                struct McPkgMutex *m, unsigned long timeout_ms);
MCPKG_API void mcpkg_cond_impl_signal(struct McPkgCond *c);
MCPKG_API void mcpkg_cond_impl_broadcast(struct McPkgCond *c);

MCPKG_END_DECLS
#endif
