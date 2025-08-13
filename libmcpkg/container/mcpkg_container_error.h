#ifndef MCPKG_CONTAINER_ERROR_H
#define MCPKG_CONTAINER_ERROR_H

#include <stdint.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS

// Per-container default hard caps (per-instance can override at *_new).
#if INTPTR_MAX > 0xFFFFFFFF
#  define MCPKG_CONTAINER_MAX_ELEMENTS          (1u << 22)  /* 4,194,304 on 64-bit */
#  define MCPKG_CONTAINER_MAX_BYTES             (2ull * 1024ull * 1024ull * 1024ull) /* 2 GiB */
#else
#  define MCPKG_CONTAINER_MAX_ELEMENTS          (1u << 20)  /* 1,048,576 on 32-bit */
#  define MCPKG_CONTAINER_MAX_BYTES             (512ull * 1024ull * 1024ull)         /* 512 MiB */
#endif

// Errors specific to containers
typedef enum {
	MCPKG_CONTAINER_OK = 0,
	MCPKG_CONTAINER_ERR_NULL_PARAM,
	MCPKG_CONTAINER_ERR_NO_MEM,
	MCPKG_CONTAINER_ERR_RANGE,
	MCPKG_CONTAINER_ERR_LIMIT,
	MCPKG_CONTAINER_ERR_OVERFLOW,
	MCPKG_CONTAINER_ERR_INVALID,
	MCPKG_CONTAINER_ERR_NOT_FOUND,
	MCPKG_CONTAINER_ERR_DUP_KEY,
	MCPKG_CONTAINER_ERR_IO,          /* reserved: for later I/O */
	MCPKG_CONTAINER_ERR_OTHER
} MCPKG_CONTAINER_ERROR;

MCPKG_END_DECLS
#endif // MCPKG_CONTAINER_ERROR_H
