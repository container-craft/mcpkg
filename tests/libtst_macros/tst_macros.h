#ifndef TST_MACROS_H
#define TST_MACROS_H

#include <stdarg.h>
#include <stddef.h>
#include <string.h> /* strcmp used by CHECK_STR */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#	include <io.h>
#	define TST_OUT_FD 1
#	define TST_ERR_FD 2
#	define TST_WRITE _write
#	define PRETTY_FUNC __FUNCDNAME__
#else
#	include <unistd.h>
#	define TST_OUT_FD STDOUT_FILENO
#	define TST_ERR_FD STDERR_FILENO
#	define TST_WRITE write
#	define PRETTY_FUNC __PRETTY_FUNCTION__
#endif

/* Global counters / verbosity */
extern int g_tst_fails;
extern int g_tst_passes;
extern int g_tst_verbose; /* 0 = quiet on pass, 1 = print [OK] lines */

/* Core logging APIs */
void tst_vlog(int fd,
              const char *prefix,
              const char *func,
              const char *file,
              const char *fmt,
              va_list ap);

void tst_fail(const char *func, const char *file, const char *fmt, ...);
void tst_pass(const char *func, const char *file, const char *fmt, ...);
void tst_info(const char *fmt, ...);

/* Verbosity / environment */
void tst_set_verbose(int on);
void tst_init_from_env(void);

/* Summary line: e.g., "pkg round-trip tests" */
void tst_summary(const char *label);

/* -------------------- Convenience helpers -------------------- */

/* Null-safe string for logging */
const char *tst_safe_str(const char *s);

/* Size helpers to avoid list type mismatches */
size_t tst_strlist_size(const void *slist); /* expects McPkgStringList* */
size_t tst_list_size(const void *list);     /* expects McPkgList*      */

/* -------------------- Assertions / checks -------------------- */

/* Prints [OK] when condition true IF g_tst_verbose != 0; always prints [FAIL] otherwise. */
#define CHECK(cond, ...) \
do { \
        if ( ( cond ) ) { \
            g_tst_passes++; \
            if ( g_tst_verbose ) { \
                tst_pass ( PRETTY_FUNC, __FILE__, __VA_ARGS__ ); \
        } \
    } else { \
            tst_fail ( PRETTY_FUNC, __FILE__, __VA_ARGS__ ); \
    } \
} while ( 0 )

/* Equality checks with caller-supplied test name; print want/got in that order. */
#define CHECK_EQ_SZ(test_name, actual, expected) \
    CHECK ( ( actual ) == ( expected ), "%s want %zu got %zu", \
          ( const char * ) ( test_name ), ( size_t ) ( expected ), ( size_t ) ( actual ) )

#define CHECK_EQ_INT(test_name, actual, expected) \
    CHECK ( ( actual ) == ( expected ), "%s want %d got %d", \
          ( const char * ) ( test_name ), ( int ) ( expected ), ( int ) ( actual ) )

/* Additional typed equality helpers */
#define CHECK_EQ_I64(test_name, actual, expected) \
    CHECK ( ( actual ) == ( expected ), "%s want %lld got %lld", \
          ( const char * ) ( test_name ), ( long long ) ( expected ), ( long long ) ( actual ) )

#define CHECK_EQ_U64(test_name, actual, expected) \
    CHECK ( ( actual ) == ( expected ), "%s want %llu got %llu", \
          ( const char * ) ( test_name ), \
          ( unsigned long long ) ( expected ), ( unsigned long long ) ( actual ) )

#define CHECK_EQ_I32(test_name, actual, expected) \
    CHECK ( ( actual ) == ( expected ), "%s want %d got %d", \
          ( const char * ) ( test_name ), ( int ) ( expected ), ( int ) ( actual ) )

#define CHECK_EQ_U32(test_name, actual, expected) \
    CHECK ( ( actual ) == ( expected ), "%s want %u got %u", \
          ( const char * ) ( test_name ), \
          ( unsigned int ) ( expected ), ( unsigned int ) ( actual ) )

/* Memory equality (memcmp) */
#define CHECK_MEMEQ(test_name, a_ptr, b_ptr, nbytes) \
    CHECK ( ( ( a_ptr ) != NULL && ( b_ptr ) != NULL && ( nbytes ) == 0 ) || \
              ( ( a_ptr ) != NULL && ( b_ptr ) != NULL && memcmp ( ( a_ptr ), ( b_ptr ), ( nbytes ) ) == 0 ), \
          "%s memcmp(%p,%p,%zu) != 0", \
          ( const char * ) ( test_name ), ( const void * ) ( a_ptr ), ( const void * ) ( b_ptr ), ( size_t ) ( nbytes ) )

/* String equality: prints want/got safely even if NULL. */
#define CHECK_STR(test_name, actual, expected) \
    do { \
        const char * _a_ = ( actual ); \
        const char * _e_ = ( expected ); \
        int _ok_ = ( _a_ && _e_ ) ? ( strcmp ( _a_, _e_ ) == 0 ) : ( _a_ == _e_ ); \
        CHECK ( _ok_, "%s want \"%s\" got \"%s\"", \
              ( const char * ) ( test_name ), \
              tst_safe_str ( _e_ ), tst_safe_str ( _a_ ) ); \
} while ( 0 )

/* Nullness helpers */
#define CHECK_NONNULL(test_name, p) \
    CHECK ( ( p ) != NULL, "%s expected non-NULL", ( const char * ) ( test_name ) )

#define CHECK_NULL(test_name, p) \
CHECK ( ( p ) == NULL, "%s expected NULL", ( const char * ) ( test_name ) )

/* Module-specific OK wrappers */
#define CHECK_CRYPTO_OK(test_name, expr) \
do { \
        MCPKG_CRYPTO_ERR _e_ = ( expr ); \
        CHECK ( _e_ == MCPKG_CRYPTO_OK, "%s err=%d", \
              ( const char * ) ( test_name ), ( int ) _e_ ); \
} while ( 0 )

#define CHECK_OKC(test_name, expr) \
do { \
        MCPKG_CONTAINER_ERROR _e_ = ( expr ); \
        CHECK ( _e_ == MCPKG_CONTAINER_OK, "%s err=%d", \
              ( const char * ) ( test_name ), ( int ) _e_ ); \
} while ( 0 )

#define CHECK_OKFS(test_name, expr) \
    do { \
            MCPKG_FS_ERROR _e_ = ( expr ); \
            CHECK ( _e_ == MCPKG_FS_OK, "%s err=%d", \
                  ( const char * ) ( test_name ), ( int ) _e_ ); \
    } while ( 0 )

/* MessagePack */
#define CHECK_OK_PACK(test_name, expr) \
do { \
            MCPKG_MP_ERROR _e_ = ( expr ); \
            CHECK ( _e_ == MCPKG_MP_NO_ERROR, "%s err=%d", \
                  ( const char * ) ( test_name ), ( int ) _e_ ); \
    } while ( 0 )

/* MC */
#define CHECK_OK_MC(test_name, expr) \
do { \
            MCPKG_MC_ERROR _e_ = ( expr ); \
            CHECK ( _e_ == MCPKG_MC_NO_ERROR, "%s err=%d", \
                  ( const char * ) ( test_name ), ( int ) _e_ ); \
} while ( 0 )

/* NET */
#define CHECK_OK_NET(test_name, expr) \
do { \
            MCPKG_NET_ERROR _e_ = ( expr ); \
            CHECK ( _e_ == MCPKG_NET_NO_ERROR, "%s err=%d", \
                  ( const char * ) ( test_name ), ( int ) _e_ ); \
} while ( 0 )

/* THREADING */
#define CHECK_OK_THREADS(test_name, expr) \
do { \
            MCPKG_THREAD_ERROR _e_ = ( expr ); \
            CHECK ( _e_ == MCPKG_THREAD_NO_ERROR, "%s err=%d", \
                ( const char * ) ( test_name ),  ( int ) _e_ ); \
} while ( 0 )


/* -------------------- List size convenience -------------------- */
/* These avoid accidental passing of McPkgStringList* to mcpkg_list_size(...) */
#define TST_STRLIST_SIZE(slptr) tst_strlist_size((const void *)(slptr))
#define TST_LIST_SIZE(lstptr)   tst_list_size((const void *)(lstptr))

#define TST_BLOCK(name, body) \
do { \
        int before_ = g_tst_fails; \
        tst_info("%s: starting...", (name)); \
        do { body } while (0); \
        if (g_tst_fails == before_) \
        tst_info("%s: OK", (name)); \
} while (0)



#ifdef __cplusplus
}
#endif

#endif /* TST_MACROS_H */
