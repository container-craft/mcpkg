#include "utils/array_helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

str_array *str_array_new() {
    str_array *arr = calloc(1, sizeof(str_array));
    if (!arr) return NULL;
    arr->capacity = 4;
    arr->elements = malloc(arr->capacity * sizeof(char*));
    if (!arr->elements) {
        free(arr);
        return NULL;
    }
    return arr;
}

int str_array_add(str_array *arr, const char *str) {
    if (arr->count >= arr->capacity) {
        size_t new_capacity = arr->capacity * 2;
        char **new_elements = realloc(arr->elements, new_capacity * sizeof(char*));
        if (!new_elements)
            return 0;
        arr->elements = new_elements;
        arr->capacity = new_capacity;
    }
    arr->elements[arr->count] = strdup(str);
    if (!arr->elements[arr->count])
        return 0;
    arr->count++;
    return 1;
}

void str_array_free(str_array *arr) {
    if (!arr) return;
    if (arr->elements) {
        for (size_t i = 0; i < arr->count; ++i) {
            free(arr->elements[i]);
        }
        free(arr->elements);
    }
    free(arr);
}

char *str_array_to_string(const str_array *arr) {
    if (!arr || arr->count == 0) {
        return strdup("[]");
    }

    // Calculate total length
    size_t total_len = 2; // For "[]"
    for (size_t i = 0; i < arr->count; ++i) {
        total_len += strlen(arr->elements[i]);
        if (i < arr->count - 1) {
            total_len += 2; // For ", "
        }
    }

    char *result = malloc(total_len + 1);
    if (!result) return NULL;

    char *ptr = result;
    *ptr++ = '[';

    for (size_t i = 0; i < arr->count; ++i) {
        strcpy(ptr, arr->elements[i]);
        ptr += strlen(arr->elements[i]);
        if (i < arr->count - 1) {
            strcpy(ptr, ", ");
            ptr += 2;
        }
    }

    *ptr++ = ']';
    *ptr = '\0';

    return result;
}
