#ifndef MCPKG_ARRAY_HELPER_H
#define MCPKG_ARRAY_HELPER_H

#include <stddef.h>
#include <stdbool.h>
#include <msgpack/pack.h>

typedef struct {
    char  **elements;
    size_t  count;
    size_t  capacity;
} str_array;

/* Build/Destroy */
str_array *str_array_new(void);
void       str_array_free(str_array *arr);

/* Append a C-string (must be NUL-terminated). Copies the string. */
int str_array_add(str_array *arr, const char *str);

/* Append from raw bytes (ptr,len). Copies and NUL-terminates. */
int str_array_add_n(str_array *arr, const char *ptr, size_t len);

/* Msgpack pack: writes an array of strings, writing nil for NULL entries */
void str_array_pack(msgpack_packer *pk, const str_array *arr);

/* Join as "[a, b, c]" (no quotes). Caller owns the returned buffer. */
char *str_array_to_string(const str_array *arr);

#endif /* MCPKG_ARRAY_HELPER_H */
