#ifndef MCPKG_MC_UTIL_H
#define MCPKG_MC_UTIL_H

#include <stddef.h>
#include "mcpkg_export.h"

MCPKG_BEGIN_DECLS
// Error codes for the mc module. 0 is success.
typedef enum {
	MCPKG_MC_NO_ERROR           = 0,     // success
	MCPKG_MC_ERR_INVALID_ARG    = -1,    // bad input parameter
	MCPKG_MC_ERR_NOT_FOUND      = -2,    // lookup failed
	MCPKG_MC_ERR_NO_MEMORY      = -3,    // allocation failure
	MCPKG_MC_ERR_PARSE          = -4,    // parse/format error
	MCPKG_MC_ERR_UNSUPPORTED    = -5,    // not supported by provider/loader
	MCPKG_MC_ERR_STATE          = -6,    // invalid state/sequence
	MCPKG_MC_ERR_IO             = -7,    // filesystem I/O error
	MCPKG_MC_ERR_OFFLINE        = -8,    // provider requires network
	MCPKG_MC_ERR_TIMEOUT        = -9,    // network or operation timeout
	MCPKG_MC_ERR_AUTH           = -10,   // auth/token failure
	MCPKG_MC_ERR_RATE_LIMIT     = -11,   // provider ratelimit exceeded
	MCPKG_MC_ERR_PROTOCOL       = -12,   // HTTP/protocol violation
	MCPKG_MC_ERR_CONFLICT       = -13,   // conflicting selection/config
	MCPKG_MC_ERR_RANGE          = -14,   // out-of-range value
} MCPKG_MC_ERROR;

// Convert an error code to a static string.
MCPKG_API const char *mcpkg_mc_errstr(MCPKG_MC_ERROR err);

MCPKG_API MCPKG_MC_ERROR mcpkg_mc_err_from_mcpkg_pack_err(int e);

// ASCII, case-insensitive equality; returns 1 if equal, 0 otherwise.
MCPKG_API int mcpkg_mc_ascii_ieq(const char *a, const char *b);

// Clamp helper for sizes; returns MCPKG_MC_NO_ERROR or ERR_RANGE.
MCPKG_API int mcpkg_mc_clamp_size(size_t min_val,
                                  size_t *io_val, size_t max_val);

// Bit helpers (no dependency)
#define MCPKG_MC_HAS_FLAG(v, f)		( ( ( v ) & ( f ) ) != 0u )
#define MCPKG_MC_SET_FLAG(v, f)		do { ( v ) |= ( f ) ; } while ( 0 )
#define MCPKG_MC_CLEAR_FLAG(v, f)     do { ( v ) &= ~ ( f ) ; } while ( 0 )

// Common string constants (static storage; never freed)
MCPKG_API const char *mcpkg_mc_string_unknown(void);     // "unknown"
MCPKG_API const char *mcpkg_mc_string_localhost(void);   // "localhost"

MCPKG_END_DECLS
#endif /* MCPKG_MC_UTIL_H */
