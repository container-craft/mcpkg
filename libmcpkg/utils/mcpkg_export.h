#ifndef MCPKG_EXPORT_H
#define MCPKG_EXPORT_H
#ifdef __cplusplus
#  define MCPKG_BEGIN_DECLS extern "C" {
#  define MCPKG_END_DECLS   }
#else
#  define MCPKG_BEGIN_DECLS
#  define MCPKG_END_DECLS
#endif
#endif /* MCPKG_EXPORT_H */
