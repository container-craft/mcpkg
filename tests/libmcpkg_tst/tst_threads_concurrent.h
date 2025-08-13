/* SPDX-License-Identifier: MIT */
#ifndef TST_THREADS_CONCURRENT_H
#define TST_THREADS_CONCURRENT_H

#include <stdint.h>
#include <stdlib.h>

#include <tst_macros.h>

#include <mcpkg_thread.h>
#include <mcpkg_thread_util.h>
#include <mcpkg_thread_future.h>
#include <mcpkg_thread_promise.h>
#include <mcpkg_thread_pool.h>

/* ---------- helpers ---------- */

struct BlockCtx {
    struct McPkgMutex	*lock;
    struct McPkgCond	*cv;
    volatile int		go;
    volatile int		ran;
};

static int task_block_until_go(void *arg)
{
    struct BlockCtx *bc = (struct BlockCtx *)arg;

    mcpkg_mutex_lock(bc->lock);
    while (!bc->go)
        mcpkg_cond_wait(bc->cv, bc->lock);
    bc->ran++;
    mcpkg_mutex_unlock(bc->lock);
    return 0;
}

struct CntCtx {
    struct McPkgMutex	*lock;
    volatile int		counter;
};

static int task_inc_counter(void *arg)
{
    struct CntCtx *cc = (struct CntCtx *)arg;
    mcpkg_mutex_lock(cc->lock);
    cc->counter++;
    mcpkg_mutex_unlock(cc->lock);
    return 0;
}

/* call() signature for pool_call_future */
static int call_make_int(void *arg, void **out_result, int *out_err)
{
    int *p = (int *)malloc(sizeof(int));
    if (!p) {
        if (out_err) *out_err = MCPKG_THREAD_E_NOMEM;
        return MCPKG_THREAD_E_NOMEM;
    }
    *p = (int)(intptr_t)arg;
    if (out_result) *out_result = p;
    if (out_err) *out_err = 0;
    return 0;
}

/* watcher counters */
static volatile int g_watch_calls = 0;
static volatile int g_watch_last_err = -1;
static volatile void *g_watch_last_res = (void *)0;

static void on_future_watch(void *user, void *result, int err)
{
    (void)user;
    g_watch_calls++;
    g_watch_last_res = result;
    g_watch_last_err = err;
}

/* producer thread args + fn */
struct ProducerArgs {
    struct McPkgThreadPromise *pr;
    int			sleep_ms;
    void			*res;
    int			err;
};

static int producer_fn(void *arg)
{
    const struct ProducerArgs *a = (const struct ProducerArgs *)arg;
    mcpkg_thread_sleep_ms((unsigned long)a->sleep_ms);
    (void)mcpkg_thread_promise_set(a->pr, a->res, a->err);
    return 0;
}

/* ---------- tests ---------- */

static void test_promise_future_basic(void)
{
    struct McPkgThreadPromise *pr = NULL;
    struct McPkgThreadFuture *f = NULL;
    struct McPkgThread *t;
    int rc;

    rc = mcpkg_thread_promise_new(&pr, &f);
    CHECK_EQ_INT("promise_new rc", rc, MCPKG_THREAD_NO_ERROR);
    CHECK_NONNULL("future nonnull", f);

    /* producer thread: sleep then resolve */
    {
        struct ProducerArgs args;
        args.pr = pr;
        args.sleep_ms = 5;
        args.res = (void *)0xCAFE;
        args.err = 0;

        t = mcpkg_thread_create(producer_fn, &args);
        CHECK_NONNULL("producer create", t);

        /* wait for result */
        {
            void *res = NULL;
            int err = -1;
            rc = mcpkg_thread_future_wait(f, 1000UL, &res, &err);
            CHECK_EQ_INT("future wait rc", rc, MCPKG_THREAD_NO_ERROR);
            CHECK_EQ_INT("future err==0", err, 0);
            CHECK ( res == (void *)0xCAFE, "future result matches" );
        }

        CHECK_OK_THREADS("join producer", mcpkg_thread_join(t));
    }

    /* clean up: consumer owns future; producer owns promise */
    mcpkg_thread_future_free(f);
    mcpkg_thread_promise_free(pr);
}

static void test_future_watch_paths(void)
{
    struct McPkgThreadFuture *f1 = mcpkg_thread_future_new();
    struct McPkgThreadFuture *f2 = mcpkg_thread_future_new();
    int rc;

    CHECK_NONNULL("f1", f1);
    CHECK_NONNULL("f2", f2);

    /* watcher before set */
    g_watch_calls = 0; g_watch_last_err = -1; g_watch_last_res = 0;
    rc = mcpkg_thread_future_watch(f1, on_future_watch, NULL);
    CHECK_EQ_INT("watch f1 rc", rc, MCPKG_THREAD_NO_ERROR);
    (void)mcpkg_thread_future_set(f1, (void *)0xBEEF, 0);
    CHECK_EQ_INT("watch calls==1", g_watch_calls, 1);
    CHECK ( g_watch_last_res == (void *)0xBEEF, "watch res f1" );
    CHECK_EQ_INT("watch err f1==0", g_watch_last_err, 0);

    /* watcher after set */
    g_watch_calls = 0; g_watch_last_err = -1; g_watch_last_res = 0;
    (void)mcpkg_thread_future_set(f2, (void *)0xFACE, 0);
    rc = mcpkg_thread_future_watch(f2, on_future_watch, NULL);
    CHECK_EQ_INT("watch f2 rc", rc, MCPKG_THREAD_NO_ERROR);
    CHECK_EQ_INT("watch calls immediate==1", g_watch_calls, 1);
    CHECK ( g_watch_last_res == (void *)0xFACE, "watch res f2" );
    CHECK_EQ_INT("watch err f2==0", g_watch_last_err, 0);

    mcpkg_thread_future_free(f1);
    mcpkg_thread_future_free(f2);
}

static void test_pool_submit_and_drain(void)
{
    struct McPkgThreadPool *p = NULL;
    struct McPkgThreadPoolCfg cfg;
    struct CntCtx cc;
    int i, rc;

    cc.lock = mcpkg_mutex_new();
    cc.counter = 0;
    CHECK_NONNULL("cnt lock", cc.lock);

    cfg.threads = 3;
    cfg.q_capacity = 16;

    rc = mcpkg_thread_pool_new(&cfg, &p);
    CHECK_EQ_INT("pool new", rc, MCPKG_THREAD_NO_ERROR);

    for (i = 0; i < 20; i++)
        CHECK_OK_THREADS("submit inc", mcpkg_thread_pool_submit(p, task_inc_counter, &cc));

    CHECK_OK_THREADS("pool drain", mcpkg_thread_pool_drain(p));
    CHECK_EQ_INT("counter==20", cc.counter, 20);

    CHECK_OK_THREADS("pool shutdown", mcpkg_thread_pool_shutdown(p));
    mcpkg_thread_pool_free(p);
    mcpkg_mutex_free(cc.lock);
}

static void test_pool_try_submit_backpressure(void)
{
    struct McPkgThreadPool *p = NULL;
    struct McPkgThreadPoolCfg cfg;
    struct BlockCtx bc;
    int rc, again;

    bc.lock = mcpkg_mutex_new();
    bc.cv = mcpkg_cond_new();
    bc.go = 0;
    bc.ran = 0;
    CHECK_NONNULL("blk lock", bc.lock);
    CHECK_NONNULL("blk cv", bc.cv);

    cfg.threads = 1;
    cfg.q_capacity = 1;

    rc = mcpkg_thread_pool_new(&cfg, &p);
    CHECK_EQ_INT("pool new", rc, MCPKG_THREAD_NO_ERROR);

    /* 1) queue one blocking task (worker will pick it up) */
    CHECK_OK_THREADS("submit block", mcpkg_thread_pool_submit(p, task_block_until_go, &bc));

    /* 2) wait until worker has dequeued it so queue is empty */
    while (mcpkg_thread_pool_queued(p) != 0)
        mcpkg_thread_sleep_ms(1);

    /* 3) now try_submit should succeed (one slot free) */
    again = mcpkg_thread_pool_try_submit(p, task_block_until_go, &bc);
    CHECK_EQ_INT("try_submit #1 ok", again, MCPKG_THREAD_NO_ERROR);

    /* 4) queue is full again → expect E_AGAIN */
    again = mcpkg_thread_pool_try_submit(p, task_block_until_go, &bc);
    CHECK_EQ_INT("try_submit #2 E_AGAIN", again, MCPKG_THREAD_E_AGAIN);

    /* release all blocked tasks */
    mcpkg_mutex_lock(bc.lock);
    bc.go = 1;
    mcpkg_cond_broadcast(bc.cv);
    mcpkg_mutex_unlock(bc.lock);

    CHECK_OK_THREADS("drain", mcpkg_thread_pool_drain(p));
    CHECK ( bc.ran >= 2, "at least two tasks ran" );

    CHECK_OK_THREADS("shutdown", mcpkg_thread_pool_shutdown(p));
    mcpkg_thread_pool_free(p);
    mcpkg_cond_free(bc.cv);
    mcpkg_mutex_free(bc.lock);
}

static void test_pool_call_future(void)
{
    struct McPkgThreadPool *p = NULL;
    struct McPkgThreadPoolCfg cfg;
    struct McPkgThreadFuture *f = NULL;
    int rc;

    cfg.threads = 2;
    cfg.q_capacity = 8;

    rc = mcpkg_thread_pool_new(&cfg, &p);
    CHECK_EQ_INT("pool new", rc, MCPKG_THREAD_NO_ERROR);

    /* ask pool to run call() and give us a future for its result */
    rc = mcpkg_thread_pool_call_future(p, call_make_int, (void *)(intptr_t)42, &f);
    CHECK_EQ_INT("call_future rc", rc, MCPKG_THREAD_NO_ERROR);
    CHECK_NONNULL("future from pool", f);

    /* wait for result */
    {
        void *res = NULL;
        int err = -1;
        rc = mcpkg_thread_future_wait(f, 1000UL, &res, &err);
        CHECK_EQ_INT("wait rc", rc, MCPKG_THREAD_NO_ERROR);
        CHECK_EQ_INT("err==0", err, 0);
        CHECK_NONNULL("res nonnull", res);
        CHECK_EQ_INT("*res==42", *(int *)res, 42);
        free(res);
    }

    mcpkg_thread_future_free(f);
    CHECK_OK_THREADS("shutdown", mcpkg_thread_pool_shutdown(p));
    mcpkg_thread_pool_free(p);
}

/* ---------- runner ---------- */

static inline void run_threads_concurrent(void)
{
    int before = g_tst_fails;

    tst_info("mcpkg threads concurrent (future/promise/pool): starting...");
    test_promise_future_basic();
    test_future_watch_paths();
    test_pool_submit_and_drain();
    test_pool_try_submit_backpressure();
    test_pool_call_future();

    if (g_tst_fails == before)
        (void)TST_WRITE(TST_OUT_FD, "mcpkg threads concurrent tests: OK\n", 35);
}

#endif /* TST_THREADS_CONCURRENT_H */
