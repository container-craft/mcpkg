/* SPDX-License-Identifier: MIT */
#ifndef TST_THREADS_BASIC_H
#define TST_THREADS_BASIC_H

#include <stdint.h>
#include <stdlib.h>

#include <tst_macros.h>
#include <mcpkg_thread.h>
#include <mcpkg_thread_util.h>

/* ---------- helpers ---------- */

struct ThreadArgs {
	struct McPkgMutex *lock;
	struct McPkgCond  *cv;
	volatile int       ready;      /* signaled under lock */
	volatile int       counter;    /* increment under lock */
	int                sleep_ms;
};

static int worker_signal_then_exit(void *arg)
{
	struct ThreadArgs *ta = (struct ThreadArgs *)arg;

	mcpkg_mutex_lock(ta->lock);
	ta->ready = 1;
	mcpkg_cond_signal(ta->cv);
	mcpkg_mutex_unlock(ta->lock);

	return 0;
}

static int worker_wait_then_inc(void *arg)
{
	struct ThreadArgs *ta = (struct ThreadArgs *)arg;

	mcpkg_mutex_lock(ta->lock);
	while (!ta->ready)
		mcpkg_cond_wait(ta->cv, ta->lock);
	ta->counter++;
	mcpkg_mutex_unlock(ta->lock);

	return 0;
}

static int worker_sleep_and_exit(void *arg)
{
	struct ThreadArgs *ta = (struct ThreadArgs *)arg;
	int ms = ta ? ta->sleep_ms : 1;
	if (ms < 0)
		ms = 0;
	mcpkg_thread_sleep_ms((unsigned long)ms);
	return 0;
}

static int worker_inc_under_lock(void *arg)
{
	struct ThreadArgs *ta = (struct ThreadArgs *)arg;
	mcpkg_mutex_lock(ta->lock);
	ta->counter++;
	mcpkg_mutex_unlock(ta->lock);
	return 0;
}

/* ---------- tests ---------- */

static void test_thread_create_join(void)
{
	struct ThreadArgs ta = {0};
	struct McPkgThread *t;

	ta.sleep_ms = 5;

	t = mcpkg_thread_create(worker_sleep_and_exit, &ta);
	CHECK_NONNULL("thread create", t);

	CHECK_OK_THREADS("thread join", mcpkg_thread_join(t));
}

static void test_thread_detach(void)
{
	struct ThreadArgs ta = {0};
	struct McPkgThread *t;

	ta.sleep_ms = 5;

	t = mcpkg_thread_create(worker_sleep_and_exit, &ta);
	CHECK_NONNULL("thread create (detach)", t);

	CHECK_OK_THREADS("thread detach", mcpkg_thread_detach(t));
}

static void test_mutex_cond_signal(void)
{
	struct ThreadArgs ta = {0};
	struct McPkgThread *t;

	ta.lock = mcpkg_mutex_new();
	ta.cv   = mcpkg_cond_new();

	CHECK_NONNULL("mutex new", ta.lock);
	CHECK_NONNULL("cond new", ta.cv);

	t = mcpkg_thread_create(worker_wait_then_inc, &ta);
	CHECK_NONNULL("worker create", t);

	/* signal */
	mcpkg_mutex_lock(ta.lock);
	ta.ready = 1;
	mcpkg_cond_signal(ta.cv);
	mcpkg_mutex_unlock(ta.lock);

	CHECK_OK_THREADS("worker join", mcpkg_thread_join(t));
	CHECK_EQ_INT("counter==1 after signal", ta.counter, 1);

	mcpkg_cond_free(ta.cv);
	mcpkg_mutex_free(ta.lock);
}

static void test_cond_timedwait_timeout(void)
{
	struct ThreadArgs ta = {0};
	int err;

	ta.lock = mcpkg_mutex_new();
	ta.cv   = mcpkg_cond_new();

	CHECK_NONNULL("mutex new", ta.lock);
	CHECK_NONNULL("cond new", ta.cv);

	mcpkg_mutex_lock(ta.lock);
	err = mcpkg_cond_timedwait(ta.cv, ta.lock, 50UL);
	mcpkg_mutex_unlock(ta.lock);

	CHECK(err == MCPKG_THREAD_E_TIMEOUT,
	      "timedwait timeout want=%d got=%d",
	      (int)MCPKG_THREAD_E_TIMEOUT, err);

	mcpkg_cond_free(ta.cv);
	mcpkg_mutex_free(ta.lock);
}

static void test_many_threads_increment(void)
{
	enum { N = 8 };
	struct ThreadArgs ta = {0};
	struct McPkgThread *th[N];
	int i;

	ta.lock = mcpkg_mutex_new();
	ta.cv   = mcpkg_cond_new();

	CHECK_NONNULL("mutex new", ta.lock);
	CHECK_NONNULL("cond new", ta.cv);

	for (i = 0; i < N; i++) {
		th[i] = mcpkg_thread_create(worker_inc_under_lock, &ta);
		CHECK_NONNULL("create worker[i]", th[i]);
	}

	for (i = 0; i < N; i++)
		CHECK_OK_THREADS("join worker[i]", mcpkg_thread_join(th[i]));

	CHECK_EQ_INT("counter==N", ta.counter, N);

	mcpkg_cond_free(ta.cv);
	mcpkg_mutex_free(ta.lock);
}

static void test_thread_id_and_name(void)
{
	uint64_t id = mcpkg_thread_id();
	CHECK(id != 0ULL, "thread id non-zero");

	/* set name may be unsupported; treat either as pass */
	{
		int e = mcpkg_thread_set_name("mcpkg-tst");
		CHECK(e == MCPKG_THREAD_NO_ERROR || e == MCPKG_THREAD_E_UNSUPPORTED,
		      "set_name ok/unsupported err=%d", e);
	}
}

/* ---------- runner ---------- */

static inline void run_threads_basic(void)
{
	int before = g_tst_fails;

	tst_info("mcpkg threads tests: starting...");
	test_thread_create_join();
	test_thread_detach();
	test_mutex_cond_signal();
	test_cond_timedwait_timeout();
	test_many_threads_increment();
	test_thread_id_and_name();

	if (g_tst_fails == before)
		(void)TST_WRITE(TST_OUT_FD, "mcpkg threads tests: OK\n", 26);
}

#endif /* TST_THREADS_BASIC_H */
