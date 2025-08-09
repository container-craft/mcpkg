#include "array_helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mcpkg.h"
static int str_array_grow(str_array *arr, size_t min_capacity) {
    size_t new_capacity = arr->capacity ? arr->capacity : 4;
    while (new_capacity < min_capacity) new_capacity <<= 1;

    char **ne = (char **)realloc(arr->elements, new_capacity * sizeof(char *));
    if (!ne) return MCPKG_ERROR_OOM;
    arr->elements = ne;
    arr->capacity = new_capacity;
    return MCPKG_ERROR_SUCCESS;
}

str_array *str_array_new(void) {
    str_array *arr = (str_array *)calloc(1, sizeof(*arr));
    if (!arr) return NULL;
    arr->capacity = 4;
    arr->elements = (char **)calloc(arr->capacity, sizeof(char *));
    if (!arr->elements) { free(arr); return NULL; }
    return arr;
}

static int str_array_add_owned(str_array *arr, char *owned) {
    if (!arr) { free(owned); return MCPKG_ERROR_PARSE; }
    if (arr->count == arr->capacity) {
        int rc = str_array_grow(arr, arr->count + 1);
        if (rc != MCPKG_ERROR_SUCCESS) { free(owned); return rc; }
    }
    arr->elements[arr->count++] = owned;
    return MCPKG_ERROR_SUCCESS;
}

int str_array_add(str_array *arr, const char *str) {
    if (!arr || !str) return MCPKG_ERROR_PARSE;
    size_t n = strlen(str);
    char *dup = (char *)malloc(n + 1);
    if (!dup) return MCPKG_ERROR_OOM;
    memcpy(dup, str, n);
    dup[n] = '\0';
    return str_array_add_owned(arr, dup);
}

int str_array_add_n(str_array *arr, const char *ptr, size_t len) {
    if (!arr || (!ptr && len)) return MCPKG_ERROR_PARSE;
    char *dup = (char *)malloc(len + 1);
    if (!dup) return MCPKG_ERROR_OOM;
    if (len) memcpy(dup, ptr, len);
    dup[len] = '\0';
    return str_array_add_owned(arr, dup);
}

void str_array_free(str_array *arr) {
    if (!arr) return;
    if (arr->elements) {
        for (size_t i = 0; i < arr->count; ++i) free(arr->elements[i]);
        free(arr->elements);
    }
    free(arr);
}

void str_array_pack(msgpack_packer *pk, const str_array *arr) {
    if (!arr) { msgpack_pack_nil(pk); return; }

    msgpack_pack_array(pk, arr->count);
    for (size_t i = 0; i < arr->count; ++i) {
        const char *s = arr->elements[i];
        if (!s) { msgpack_pack_nil(pk); continue; }
        size_t n = strlen(s);
        msgpack_pack_str(pk, n);
        msgpack_pack_str_body(pk, s, n);
    }
}

char *str_array_to_string(const str_array *arr) {
    if (!arr || arr->count == 0) return strdup("[]");

    /* First pass: sum lengths and count non-NULL items */
    size_t sum = 0, nn = 0;
    for (size_t i = 0; i < arr->count; ++i) {
        const char *s = arr->elements[i];
        if (!s) continue;
        sum += strlen(s);
        nn++;
    }
    if (nn == 0) return strdup("[]");

    /* Capacity: [ + items + ", "*(nn-1) + ] + NUL */
    size_t cap = 1 + sum + (nn > 1 ? 2 * (nn - 1) : 0) + 1 + 1;
    char *out = (char *)malloc(cap);
    if (!out) return NULL;

    char *p = out;
    *p++ = '[';

    size_t seen = 0;
    for (size_t i = 0; i < arr->count; ++i) {
        const char *s = arr->elements[i];
        if (!s) continue;
        if (seen++) { *p++ = ','; *p++ = ' '; }
        size_t n = strlen(s);
        memcpy(p, s, n);
        p += n;
    }

    *p++ = ']';
    *p = '\0';
    return out;
}
