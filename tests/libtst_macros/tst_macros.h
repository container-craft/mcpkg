#ifndef TST_MACROS_H
#define TST_MACROS_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
        #include <io.h>
        #define TST_OUT_FD 1
        #define TST_ERR_FD 2
        #define TST_WRITE  _write
        #define PRETTY_FUNC __FUNCDNAME__
#else
        #include <unistd.h>
        #define TST_OUT_FD STDOUT_FILENO
        #define TST_ERR_FD STDERR_FILENO
        #define TST_WRITE  write
        #define PRETTY_FUNC __PRETTY_FUNCTION__
#endif

extern int g_tst_fails;
extern int g_tst_passes;
extern int g_tst_verbose;    // 0 = quiet on pass, 1 = print [OK] lines

// fd/prefix + function + file;
void tst_vlog ( int fd,
              const char *prefix,
              const char *func,
              const char *file,
              const char *fmt,
              va_list ap );

void tst_fail ( const char *func, const char *file, const char *fmt, ... );
void tst_pass ( const char *func, const char *file, const char *fmt, ... );
void tst_info ( const char *fmt, ... );

// Set verbosity explicitly or via env (see .c)
void tst_set_verbose ( int on );
void tst_init_from_env ( void );

void tst_summary(const char *label);

// Prints [OK] when condition true IF g_tst_verbose != 0; always prints [FAIL] otherwise.
#define CHECK(cond, ...) \
do { \
        if ( ( cond ) ) { \
            if ( g_tst_verbose ) \
            tst_pass ( PRETTY_FUNC , __FILE__ , __VA_ARGS__ ); \
    } else { \
            tst_fail ( PRETTY_FUNC , __FILE__ , __VA_ARGS__ ); \
    } \
} while ( 0 )

// Equality checks with caller-supplied test name; print want/got in that order.
#define CHECK_EQ_SZ(test_name, actual, expected) \
    CHECK ( ( actual ) == ( expected ), "%s want %zu got %zu", \
          (const char *) ( test_name ), (size_t) ( expected ), (size_t) ( actual ) )

#define CHECK_EQ_INT(test_name, actual, expected) \
    CHECK ( ( actual ) == ( expected ), "%s want %d got %d", \
          (const char *) ( test_name ), (int) ( expected ), (int) ( actual ) )

// String equality: prints want/got safely even if NULL.
#define CHECK_STR(test_name, actual, expected) \
do { \
    const char *_a_ = ( actual ) ; \
    const char *_e_ = ( expected ) ; \
    int _ok_ = ( _a_  &&  _e_ )  ?  ( strcmp ( _a_ , _e_ ) == 0 ) :  ( _a_  ==  _e_ ) ; \
        CHECK( _ok_ , "%s want \"%s\" got \"%s\"" , \
        ( const char * ) ( test_name ) , \
              ( _e_  ?  _e_  :  "(null)" ) , \
              ( _a_ ? _a_  : "(null)" )  ) ; \
} while ( 0 )

// MODULES

#define CHECK_CRYPTO_OK(test_name, expr) \
do { \
        MCPKG_CRYPTO_ERR _e_ = ( expr ); \
        CHECK( _e_ == MCPKG_CRYPTO_OK, "%s err=%d", \
              ( const char * ) ( test_name ) , ( int ) _e_ ); \
} while ( 0 )


#define CHECK_OKC(test_name, expr) \
    do { \
        MCPKG_CONTAINER_ERROR _e_ = ( expr ) ; \
        CHECK ( _e_ == MCPKG_CONTAINER_OK, "%s err=%d", \
              ( const char * ) ( test_name ), ( int ) _e_ ) ; \
} while ( 0 )


#define CHECK_OKFS(test_name, expr) \
    do { \
            MCPKG_FS_ERROR _e_ = ( expr ); \
            CHECK ( _e_ == MCPKG_FS_OK , "%s err=%d", \
                  ( const char * ) ( test_name ), ( int ) _e_ ); \
    } while ( 0 )


#ifdef __cplusplus
}
#endif

#endif /* TST_MACROS_H */
