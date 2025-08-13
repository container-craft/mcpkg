#ifndef MCPKG_THREAD_UTIL_H
#define MCPKG_THREAD_UTIL_H

#include <stdint.h>
#include <stddef.h>
#include <mcpkg_export.h>
MCPKG_BEGIN_DECLS
typedef enum {
	MCPKG_THREAD_NO_ERROR        = 0,
	MCPKG_THREAD_E_UNSUPPORTED   = 1,
	MCPKG_THREAD_E_SYS           = 2,
	MCPKG_THREAD_E_INVAL         = 3,
	MCPKG_THREAD_E_AGAIN         = 4,
	MCPKG_THREAD_E_NOMEM         = 5,
	MCPKG_THREAD_E_TIMEOUT       = 6,
} MCPKG_THREAD_ERROR;


MCPKG_API void mcpkg_thread_sleep_ms(unsigned long ms);
MCPKG_API uint64_t mcpkg_thread_time_ms(void);

MCPKG_END_DECLS
#endif
