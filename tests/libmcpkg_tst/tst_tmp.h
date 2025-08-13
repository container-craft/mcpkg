#ifndef TST_XXXX_H
#define TST_XXXX_H

//other modules

// Test module (example net/)

// Test macros
#include <tst_macros.h>

static void test_XXXX(void) {}
static void test_XXXXX(void) {}

static inline void run_net_XXXX(void)
{
    int before = g_tst_fails;
    tst_info("mcpkg XXXX tests: starting...");
    test_XXXX();
    test_XXXXX();
    if (g_tst_fails == before)
        (void)TST_WRITE(TST_OUT_FD,
                         "mcpkg XXXX tests: OK\n",
                         31);
}
#endif // TST_XXXX_H
