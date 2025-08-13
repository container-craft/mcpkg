#ifndef MCPKG_EXPORT_H
#define MCPKG_EXPORT_H

#ifdef __cplusplus
#  define MCPKG_BEGIN_DECLS extern "C" {
#  define MCPKG_END_DECLS   }
#else
#  define MCPKG_BEGIN_DECLS
#  define MCPKG_END_DECLS
#endif


#if defined(__GNUC__) || defined(__clang__)
#define MCPKG_ATTR_UNUSED __attribute__((unused))
#else
#define MCPKG_ATTR_UNUSED
#endif

/* MCPKG_API:
 * Define MCPKG_BUILD_SHARED when compiling the shared lib.
 * Define MCPKG_BUILDING_LIBRARY within the library itself.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(MCPKG_BUILD_SHARED)
#    if defined(MCPKG_BUILDING_LIBRARY)
#      define MCPKG_API __declspec(dllexport)
#    else
#      define MCPKG_API __declspec(dllimport)
#    endif
#  else
#    define MCPKG_API
#  endif
#  define MCPKG_LOCAL
#else
#  if defined(MCPKG_BUILD_SHARED)
#    if __GNUC__ >= 4
#      define MCPKG_API   __attribute__((visibility("default")))
#      define MCPKG_LOCAL __attribute__((visibility("hidden")))
#    else
#      define MCPKG_API
#      define MCPKG_LOCAL
#    endif
#  else
#    define MCPKG_API
#    define MCPKG_LOCAL
#  endif
#endif

#endif /* MCPKG_EXPORT_H */
