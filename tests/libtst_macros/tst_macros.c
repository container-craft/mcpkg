/* SPDX-License-Identifier: MIT */
#include "tst_macros.h"

#include <stdio.h>
#include <stdlib.h>

#include <container/mcpkg_list.h>
#include <container/mcpkg_str_list.h>

int g_tst_fails;
int g_tst_passes;
int g_tst_verbose;

static void tst_write_line(int fd,
                           const char *prefix,
                           const char *func,
                           const char *file,
                           const char *line)
{
    /* Format: [PREFIX] func file: line\n */
    char buf[1024];
    int n;

    if (!prefix)
        prefix = "";
    if (!func)
        func = "";
    if (!file)
        file = "";

    n = snprintf(buf, sizeof(buf), "[%s] %s %s: %s\n",
                 prefix, func, file, line ? line : "");
    if (n < 0)
        return;
    if (n > (int)sizeof(buf))
        n = (int)sizeof(buf);

    (void)TST_WRITE(fd, buf, (size_t)n);
}

void tst_vlog(int fd,
              const char *prefix,
              const char *func,
              const char *file,
              const char *fmt,
              va_list ap)
{
    char msg[896];
    int n;

    if (!fmt) {
        tst_write_line(fd, prefix, func, file, "");
        return;
    }

    n = vsnprintf(msg, sizeof(msg), fmt, ap);
    if (n < 0) {
        tst_write_line(fd, prefix, func, file, "(format error)");
        return;
    }
    if (n >= (int)sizeof(msg))
        msg[sizeof(msg) - 1] = '\0';

    tst_write_line(fd, prefix, func, file, msg);
}

void tst_fail(const char *func, const char *file, const char *fmt, ...)
{
    va_list ap;

    g_tst_fails++;

    va_start(ap, fmt);
    tst_vlog(TST_ERR_FD, "FAIL", func, file, fmt, ap);
    va_end(ap);
}

void tst_pass(const char *func, const char *file, const char *fmt, ...)
{
    va_list ap;

    // g_tst_passes++;

    if (!g_tst_verbose)
        return;

    va_start(ap, fmt);
    tst_vlog(TST_OUT_FD, "OK", func, file, fmt, ap);
    va_end(ap);
}

void tst_info(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if(g_tst_verbose)
        tst_vlog(TST_OUT_FD, "INFO", PRETTY_FUNC, __FILE__, fmt, ap);

    va_end(ap);
}

void tst_set_verbose(int on)
{
    g_tst_verbose = on ? 1 : 0;
}

void tst_init_from_env(void)
{
    if(TST_VERBOSE)
            g_tst_verbose = 1;
}

void tst_summary(const char *label)
{
    char line[128];
    int n;

    n = snprintf(line, sizeof(line),
                 "%s: Failed: %d Passed: %d Total: %d",
                 label ? label : "Test Results",
                 g_tst_fails, g_tst_passes, g_tst_fails + g_tst_passes);
    if (n < 0)
        return;
    if (n > (int)sizeof(line))
        n = (int)sizeof(line);

    (void)TST_WRITE(TST_OUT_FD, line, (size_t)n);
    (void)TST_WRITE(TST_OUT_FD, "\n", 1);
}

/* -------------------- Convenience helpers -------------------- */

const char *tst_safe_str(const char *s)
{
    return s ? s : "(null)";
}

size_t tst_strlist_size(const void *slist)
{
    const struct McPkgStringList *sl = (const struct McPkgStringList *)slist;

    return sl ? mcpkg_stringlist_size(sl) : 0u;
}

size_t tst_list_size(const void *list)
{
    const struct McPkgList *lst = (const struct McPkgList *)list;

    return lst ? mcpkg_list_size(lst) : 0u;
}
