/* SPDX-License-Identifier: MIT */
#include "../mcpkg_thread_util.h"
#include "mcpkg_thread_posix.h"

#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__)
#   include <sys/syscall.h>
#endif

struct start_pack {
	int (*fn)(void *);
	void *arg;
};

static void *start_thunk(void *p)
{
	struct start_pack *sp = (struct start_pack *)p;
	int (*fn)(void *) = sp->fn;
	void *arg = sp->arg;
	free(sp);
	(void)fn(arg);
	return NULL;
}

struct McPkgThread *mcpkg_thread_impl_create(int (*fn)(void *), void *arg)
{
	struct McPkgThread *t;
	struct start_pack *sp;

	t = calloc(1, sizeof(*t));
	if (!t) return NULL;
	sp = malloc(sizeof(*sp));
	if (!sp) {
		free(t);
		return NULL;
	}
	sp->fn = fn;
	sp->arg = arg;

	if (pthread_create(&t->t, NULL, start_thunk, sp) != 0) {
		free(sp);
		free(t);
		return NULL;
	}
	return t;
}

int mcpkg_thread_impl_join(struct McPkgThread *t)
{
	if (!t) return MCPKG_THREAD_E_INVAL;
	if (t->detached) return MCPKG_THREAD_E_INVAL;
	if (t->joined) return MCPKG_THREAD_NO_ERROR;
	if (pthread_join(t->t, NULL) != 0) return MCPKG_THREAD_E_SYS;
	t->joined = 1;
	free(t);
	return MCPKG_THREAD_NO_ERROR;
}

int mcpkg_thread_impl_detach(struct McPkgThread *t)
{
	if (!t) return MCPKG_THREAD_E_INVAL;
	if (t->joined) {
		free(t);
		return MCPKG_THREAD_NO_ERROR;
	}
	if (!t->detached && pthread_detach(t->t) != 0) return MCPKG_THREAD_E_SYS;
	t->detached = 1;
	free(t);
	return MCPKG_THREAD_NO_ERROR;
}

uint64_t mcpkg_thread_impl_id(void)
{
#if defined(__linux__) && defined(SYS_gettid)
	/* Kernel TID is stable and fits in u64 */
	return (uint64_t)syscall(SYS_gettid);
#else
	/* Strictly POSIX fallback: hash pthread_t bytes into a u64 */
	pthread_t self = pthread_self();
	uint64_t id = 0;
	size_t n = sizeof(self) < sizeof(id) ? sizeof(self) : sizeof(id);
	memcpy(&id, &self, n);
	return id;
#endif
}

int mcpkg_thread_impl_set_name(const char *name)
{
#if defined(__linux__)
	if (!name)
		return MCPKG_THREAD_E_INVAL;
	char buf[16];
	size_t i = 0;
	while (name[i] && i < sizeof(buf) - 1) {
		buf[i] = name[i];
		i++;
	}
	buf[i] = '\0';
	return pthread_setname_np(pthread_self(), buf) == 0
	       ? MCPKG_THREAD_NO_ERROR
	       : MCPKG_THREAD_E_SYS;
#else
	(void)name;
	return MCPKG_THREAD_E_UNSUPPORTED;
#endif
}

/* Mutex */
struct McPkgMutex *mcpkg_mutex_impl_new(void)
{
	struct McPkgMutex *m = malloc(sizeof(*m));
	if (!m) return NULL;
	if (pthread_mutex_init(&m->m, NULL) != 0) {
		free(m);
		return NULL;
	}
	return m;
}
void mcpkg_mutex_impl_free(struct McPkgMutex *m)
{
	if (m) {
		pthread_mutex_destroy(&m->m);
		free(m);
	}
}
void mcpkg_mutex_impl_lock(struct McPkgMutex *m)
{
	pthread_mutex_lock(&m->m);
}
void mcpkg_mutex_impl_unlock(struct McPkgMutex *m)
{
	pthread_mutex_unlock(&m->m);
}

/* Condvar */
struct McPkgCond *mcpkg_cond_impl_new(void)
{
	struct McPkgCond *c = malloc(sizeof(*c));
	if (!c) return NULL;
	if (pthread_cond_init(&c->c, NULL) != 0) {
		free(c);
		return NULL;
	}
	return c;
}
void mcpkg_cond_impl_free(struct McPkgCond *c)
{
	if (c) {
		pthread_cond_destroy(&c->c);
		free(c);
	}
}
void mcpkg_cond_impl_wait(struct McPkgCond *c, struct McPkgMutex *m)
{
	pthread_cond_wait(&c->c, &m->m);
}

int mcpkg_cond_impl_timedwait(struct McPkgCond *c, struct McPkgMutex *m,
                              unsigned long timeout_ms)
{
	struct timespec ts;
#if defined(CLOCK_MONOTONIC)
	clock_gettime(CLOCK_MONOTONIC, &ts);
#else
	clock_gettime(CLOCK_REALTIME, &ts);
#endif
	ts.tv_sec  += (time_t)(timeout_ms / 1000UL);
	ts.tv_nsec += (long)((timeout_ms % 1000UL) * 1000000UL);
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}
	int r = pthread_cond_timedwait(&c->c, &m->m, &ts);
	if (r == 0) return MCPKG_THREAD_NO_ERROR;
	if (r == ETIMEDOUT) return MCPKG_THREAD_E_TIMEOUT;
	return MCPKG_THREAD_E_SYS;
}

void mcpkg_cond_impl_signal(struct McPkgCond *c)
{
	pthread_cond_signal(&c->c);
}
void mcpkg_cond_impl_broadcast(struct McPkgCond *c)
{
	pthread_cond_broadcast(&c->c);
}
