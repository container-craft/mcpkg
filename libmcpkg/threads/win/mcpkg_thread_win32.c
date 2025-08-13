/* SPDX-License-Identifier: MIT */
#include "threads/mcpkg_thread_util.h"
#include "mcpkg_thread_win32.h"

#include <process.h> /* _beginthreadex */

struct start_pack {
	int (*fn)(void *);
	void *arg;
};

static unsigned __stdcall start_thunk(void *p)
{
	struct start_pack *sp = (struct start_pack *)p;
	int (*fn)(void *) = sp->fn;
	void *arg = sp->arg;
	free(sp);
	(void)fn(arg);
	return 0U;
}

struct McPkgThread *mcpkg_thread_impl_create(int (*fn)(void *), void *arg)
{
	struct McPkgThread *t = (struct McPkgThread *)calloc(1, sizeof(*t));
	struct start_pack *sp = (struct start_pack *)malloc(sizeof(*sp));
	if (!t || !sp) {
		free(t);
		free(sp);
		return NULL;
	}
	sp->fn = fn;
	sp->arg = arg;

	t->h = (HANDLE)_beginthreadex(NULL, 0, start_thunk, sp, 0, (unsigned *)&t->tid);
	if (!t->h) {
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
	if (t->joined) {
		CloseHandle(t->h);
		free(t);
		return MCPKG_THREAD_NO_ERROR;
	}
	DWORD r = WaitForSingleObject(t->h, INFINITE);
	if (r != WAIT_OBJECT_0) return MCPKG_THREAD_E_SYS;
	CloseHandle(t->h);
	t->joined = 1;
	free(t);
	return MCPKG_THREAD_NO_ERROR;
}

int mcpkg_thread_impl_detach(struct McPkgThread *t)
{
	if (!t) return MCPKG_THREAD_E_INVAL;
	if (!t->detached) {
		CloseHandle(t->h);
		t->detached = 1;
	}
	free(t);
	return MCPKG_THREAD_NO_ERROR;
}

uint64_t mcpkg_thread_impl_id(void)
{
	return (uint64_t)GetCurrentThreadId();
}

int mcpkg_thread_impl_set_name(const char *name)
{
	/* Windows 10+: SetThreadDescription (UTF‑16) */
	typedef HRESULT(WINAPI * pSetThreadDescription)(HANDLE, PCWSTR);
	static pSetThreadDescription setdesc = NULL;
	static int looked = 0;

	if (!looked) {
		HMODULE h = GetModuleHandleW(L"Kernel32.dll");
		setdesc = (pSetThreadDescription)GetProcAddress(h, "SetThreadDescription");
		looked = 1;
	}
	if (!setdesc || !name) return MCPKG_THREAD_E_UNSUPPORTED;

	/* naive UTF‑8 -> UTF‑16 */
	int wlen = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
	if (wlen <= 0) return MCPKG_THREAD_E_INVAL;
	WCHAR *w = (WCHAR *)malloc((size_t)wlen * sizeof(WCHAR));
	if (!w) return MCPKG_THREAD_E_NOMEM;
	MultiByteToWideChar(CP_UTF8, 0, name, -1, w, wlen);
	HRESULT hr = setdesc(GetCurrentThread(), w);
	free(w);
	return SUCCEEDED(hr) ? MCPKG_THREAD_NO_ERROR : MCPKG_THREAD_E_SYS;
}

/* Mutex */
struct McPkgMutex *mcpkg_mutex_impl_new(void)
{
	struct McPkgMutex *m = (struct McPkgMutex *)malloc(sizeof(*m));
	if (!m) return NULL;
	InitializeCriticalSection(&m->cs);
	return m;
}
void mcpkg_mutex_impl_free(struct McPkgMutex *m)
{
	if (m) {
		DeleteCriticalSection(&m->cs);
		free(m);
	}
}
void mcpkg_mutex_impl_lock(struct McPkgMutex *m)
{
	EnterCriticalSection(&m->cs);
}
void mcpkg_mutex_impl_unlock(struct McPkgMutex *m)
{
	LeaveCriticalSection(&m->cs);
}

/* Condvar */
struct McPkgCond *mcpkg_cond_impl_new(void)
{
	struct McPkgCond *c = (struct McPkgCond *)malloc(sizeof(*c));
	if (!c) return NULL;
	InitializeConditionVariable(&c->cv);
	return c;
}
void mcpkg_cond_impl_free(struct McPkgCond *c)
{
	if (c) free(c);
}

void mcpkg_cond_impl_wait(struct McPkgCond *c, struct McPkgMutex *m)
{
	SleepConditionVariableCS(&c->cv, &m->cs, INFINITE);
}

int mcpkg_cond_impl_timedwait(struct McPkgCond *c, struct McPkgMutex *m,
                              unsigned long timeout_ms)
{
	BOOL ok = SleepConditionVariableCS(&c->cv, &m->cs, timeout_ms);
	if (ok) return MCPKG_THREAD_NO_ERROR;
	return (GetLastError() == ERROR_TIMEOUT) ? MCPKG_THREAD_E_TIMEOUT :
	       MCPKG_THREAD_E_SYS;
}

void mcpkg_cond_impl_signal(struct McPkgCond *c)
{
	WakeConditionVariable(&c->cv);
}
void mcpkg_cond_impl_broadcast(struct McPkgCond *c)
{
	WakeAllConditionVariable(&c->cv);
}
