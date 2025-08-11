#ifndef MCPKG_MSGPACK_H
#define MCPKG_MSGPACK_H
#include <stdarg.h>

#include "array_helper.h"
#include "mcpkg.h"

#include "utils/mcpkg_export.h"
MCPKG_BEGIN_DECLS

static inline const char *mcpkg_safe_str(const char *s)
{
    return s ? s : "(null)";
}

static inline char *strndup_safe(const char *s,
                 size_t n)
{
    if (!s || n == 0)
        return NULL;
    char *out = malloc(n + 1);
    if (!out)
        return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static int equal_key(msgpack_object k,
                     const char *lit)
{
    size_t n = strlen(lit);
    return (k.type == MSGPACK_OBJECT_STR &&
            k.via.str.size == n &&
            memcmp(k.via.str.ptr, lit, n) == 0);
}

static void pack_str_or_nil(msgpack_packer *pk,
                            const char *s)
{
    if (!s) {
        msgpack_pack_nil(pk);
        return;
    }
    size_t n = strlen(s);
    msgpack_pack_str(pk, n);
    msgpack_pack_str_body(pk, s, n);
}

static MCPKG_ERROR_TYPE appendf(char **dst,
                                const char *fmt,
                                ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *add = NULL;
    int nw = vasprintf(&add, fmt, ap);
    va_end(ap);
    if (nw < 0)
        return MCPKG_ERROR_PARSE;

    if (!*dst) {
        *dst = add;
    } else {
        size_t oldlen = strlen(*dst);
        char *tmp = (char *)realloc(*dst, oldlen + (size_t)nw + 1);
        if (!tmp) {
            free(add);
            return MCPKG_ERROR_OOM;
        }
        memcpy(tmp + oldlen, add, (size_t)nw + 1);
        free(add);
        *dst = tmp;
    }
    return MCPKG_ERROR_SUCCESS;
}

//TODO have this return out error type
static MCPKG_ERROR_TYPE str_array_add_from_msgpack_str(
    str_array *arr,
    const msgpack_object_str s)
{
    if (!arr || !s.ptr || s.size == 0)
        return MCPKG_ERROR_OOM;

    char *tmp = (char *)malloc(s.size + 1);
    if (!tmp)
        return MCPKG_ERROR_OOM;

    memcpy(tmp, s.ptr, s.size);
    tmp[s.size] = '\0';
    /* str_array_add duplicates; free our temp afterwards */
    str_array_add(arr, tmp);
    free(tmp);
    return MCPKG_ERROR_SUCCESS;
}


MCPKG_END_DECLS
#endif
