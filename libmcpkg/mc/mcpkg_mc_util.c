// Shared utilities and error handling for the mc module.

#include <string.h>
#include "mcpkg_mc_util.h"
#include "pack/mcpkg_msgpack.h"

// Static strings for reuse.
static const char *const s_unknown   = "unknown";
static const char *const s_localhost = "localhost";

// Error string table.
struct mc_err_entry {
    MCPKG_MC_ERROR code;
    const char *msg;
};

static const struct mc_err_entry mc_err_table[] = {
    { MCPKG_MC_NO_ERROR,         "no error" },
    { MCPKG_MC_ERR_INVALID_ARG,  "invalid argument" },
    { MCPKG_MC_ERR_NOT_FOUND,    "not found" },
    { MCPKG_MC_ERR_NO_MEMORY,    "out of memory" },
    { MCPKG_MC_ERR_PARSE,        "parse error" },
    { MCPKG_MC_ERR_UNSUPPORTED,  "unsupported" },
    { MCPKG_MC_ERR_STATE,        "invalid state" },
    { MCPKG_MC_ERR_IO,           "I/O error" },
    { MCPKG_MC_ERR_OFFLINE,      "offline" },
    { MCPKG_MC_ERR_TIMEOUT,      "timeout" },
    { MCPKG_MC_ERR_AUTH,         "authentication failed" },
    { MCPKG_MC_ERR_RATE_LIMIT,   "rate limit exceeded" },
    { MCPKG_MC_ERR_PROTOCOL,     "protocol error" },
    { MCPKG_MC_ERR_CONFLICT,     "conflict" },
    { MCPKG_MC_ERR_RANGE,        "out of range" },
    };

const char *mcpkg_mc_errstr(MCPKG_MC_ERROR err)
{
    size_t i;

    for (i = 0; i < sizeof(mc_err_table) / sizeof(mc_err_table[0]); i++) {
        if (mc_err_table[i].code == err)
            return mc_err_table[i].msg;
    }
    return s_unknown;
}
// whatever .....
MCPKG_MC_ERROR mcpkg_mc_err_from_mcpkg_pack_err(int e)
{
    switch (e) {
    case MCPKG_MP_NO_ERROR:        return MCPKG_MC_NO_ERROR;
    case MCPKG_MP_ERR_INVALID_ARG: return MCPKG_MC_ERR_INVALID_ARG;
    case MCPKG_MP_ERR_PARSE:       return MCPKG_MC_ERR_PARSE;
    case MCPKG_MP_ERR_NO_MEMORY:   return MCPKG_MC_ERR_NO_MEMORY;
    case MCPKG_MP_ERR_IO:          return MCPKG_MC_ERR_IO;
    default:                       return MCPKG_MC_ERR_PARSE;
    }
}

int mcpkg_mc_ascii_ieq(const char *a, const char *b)
{
    if (!a || !b)
        return 0;
    while (*a && *b) {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        if ((ca >= 'A' && ca <= 'Z') ? (ca + 32) != cb :
                (cb >= 'A' && cb <= 'Z') ? (cb + 32) != ca :
                ca != cb)
            return 0;
    }
    return *a == '\0' && *b == '\0';
}

int mcpkg_mc_clamp_size(size_t min_val, size_t *io_val, size_t max_val)
{
    if (!io_val)
        return MCPKG_MC_ERR_INVALID_ARG;
    if (*io_val < min_val) {
        *io_val = min_val;
        return MCPKG_MC_ERR_RANGE;
    }
    if (*io_val > max_val) {
        *io_val = max_val;
        return MCPKG_MC_ERR_RANGE;
    }
    return MCPKG_MC_NO_ERROR;
}

const char *mcpkg_mc_string_unknown(void)
{
    return s_unknown;
}

const char *mcpkg_mc_string_localhost(void)
{
    return s_localhost;
}
