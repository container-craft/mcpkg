#include "mcpkg_thread.h"
#if defined(_WIN32) && !defined(__CYGWIN__)
#include "win/mcpkg_thread_win32.h"
#else
#include "posix/mcpkg_thread_posix.h"
#endif

struct McPkgThread *mcpkg_thread_create(mcpkg_thread_fn fn, void *arg)
{
	return mcpkg_thread_impl_create(fn, arg);
}

int mcpkg_thread_join(struct McPkgThread *t)
{
	return mcpkg_thread_impl_join(t);
}

int mcpkg_thread_detach(struct McPkgThread *t)
{
	return mcpkg_thread_impl_detach(t);
}

uint64_t mcpkg_thread_id(void)
{
	return mcpkg_thread_impl_id();
}

int mcpkg_thread_set_name(const char *name)
{
	return mcpkg_thread_impl_set_name(name);
}

/* ---------- mutex ---------- */

struct McPkgMutex *mcpkg_mutex_new(void)
{
	return mcpkg_mutex_impl_new();
}

void mcpkg_mutex_free(struct McPkgMutex *m)
{
	mcpkg_mutex_impl_free(m);
}

void mcpkg_mutex_lock(struct McPkgMutex *m)
{
	mcpkg_mutex_impl_lock(m);
}

void mcpkg_mutex_unlock(struct McPkgMutex *m)
{
	mcpkg_mutex_impl_unlock(m);
}

/* ---------- condition variable ---------- */

struct McPkgCond *mcpkg_cond_new(void)
{
	return mcpkg_cond_impl_new();
}

void mcpkg_cond_free(struct McPkgCond *c)
{
	mcpkg_cond_impl_free(c);
}

void mcpkg_cond_wait(struct McPkgCond *c, struct McPkgMutex *m)
{
	mcpkg_cond_impl_wait(c, m);
}

int mcpkg_cond_timedwait(struct McPkgCond *c, struct McPkgMutex *m,
                         unsigned long timeout_ms)
{
	return mcpkg_cond_impl_timedwait(c, m, timeout_ms);
}

void mcpkg_cond_signal(struct McPkgCond *c)
{
	mcpkg_cond_impl_signal(c);
}

void mcpkg_cond_broadcast(struct McPkgCond *c)
{
	mcpkg_cond_impl_broadcast(c);
}
