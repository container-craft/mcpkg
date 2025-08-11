#include "tst_macros.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int g_tst_fails   = 0;
int g_tst_passes  = 0;

#ifndef TST_VERBOSE_DEFAULT
#  define TST_VERBOSE_DEFAULT 0
#endif
int g_tst_verbose = TST_VERBOSE_DEFAULT;

void tst_vlog ( int fd,
              const char *prefix,
              const char *func,
              const char *file,
              const char *fmt,
              va_list ap )
{
    char buf[512];
    size_t off = 0;
    int n;

    if ( prefix == NULL ) prefix = "";
    if ( func   == NULL ) func   = "(func)";
    if ( file   == NULL ) file   = "(file)";
    if ( fmt    == NULL ) fmt    = "(null)";

    n = snprintf ( buf, sizeof buf, "%s %s: %s: ",
                 prefix, file, func );
    if ( n < 0 ) return;
    off = ( (size_t) n < sizeof buf ) ? (size_t) n : ( sizeof buf - 1 );

    n = vsnprintf ( buf + off, sizeof buf - off, fmt, ap );
    if ( n < 0 ) n = 0;
    off += (size_t) n;
    if ( off >= sizeof buf ) off = sizeof buf - 1;

    buf[off++] = '\n';
    (void) TST_WRITE ( fd, buf, (unsigned int) off );
}

void tst_fail ( const char *func, const char *file, const char *fmt, ... )
{
    va_list ap;
    va_start ( ap, fmt );
    tst_vlog ( TST_ERR_FD, "[FAIL]", func, file, fmt, ap );
    va_end ( ap );
    g_tst_fails++;
}

void tst_pass ( const char *func, const char *file, const char *fmt, ... )
{
    va_list ap;
    g_tst_passes++;
    if (g_tst_verbose){
        va_start ( ap, fmt );
        tst_vlog ( TST_OUT_FD, "[OK]"  , func, file, fmt, ap );
        va_end ( ap );
    }
}

void tst_info ( const char *fmt, ... )
{
    va_list ap;
    va_start ( ap, fmt );
    /* Note: PRETTY_FUNC here resolves to this function’s pretty name.
     * If you want caller’s function name, we can add a TST_INFO(...) macro wrapper.
     */
    tst_vlog ( TST_OUT_FD, "[INFO]", PRETTY_FUNC, __FILE__, fmt, ap );
    va_end ( ap );
}

void tst_set_verbose ( int on )
{
    g_tst_verbose = on ? 1 : 0;
}

void tst_init_from_env ( void )
{
    const char *v = getenv ( "TST_VERBOSE" );
    if ( v && ( *v == '1' || *v == 'y' || *v == 'Y' ||
              *v == 't' || *v == 'T' ) )
        g_tst_verbose = 1;
}

void tst_summary(const char *label)
{
    char buf[160];
    int total = g_tst_passes + g_tst_fails;
    int n;

    if (label && *label){
        n = snprintf(buf, sizeof buf,
                     "Test Results (%s): Failed: %d Passed: %d Total: %d\n",
                     label, g_tst_fails, g_tst_passes, total);
    }else{
        n = snprintf(buf, sizeof buf,
                     "Test Results: Failed:  %d  Passed: %d  Total : %d\n",
                     g_tst_fails, g_tst_passes, total);
    }
    if (n < 0)
        return;
    if ((size_t) n > sizeof buf)
        n = (int)sizeof buf;
    (void)TST_WRITE(TST_OUT_FD, buf, (unsigned int)n);
}
