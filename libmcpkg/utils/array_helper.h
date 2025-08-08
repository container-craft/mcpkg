#ifndef MCPKG_ARRAY_HELPER_H
#define MCPKG_ARRAY_HELPER_H

#include <msgpack/pack.h>
#include <stddef.h>
#include <stdbool.h>

// A dynamically allocated string array.
typedef struct {
    char **elements;
    size_t count;
    size_t capacity;
} str_array;


/**
 * @brief
 * @return
 */
static void str_array_pack(msgpack_packer *pk, const str_array *arr) {
    msgpack_pack_array(pk, arr->count);
    for (size_t i = 0; i < arr->count; ++i) {
        msgpack_pack_str_with_body(pk, arr->elements[i], strlen(arr->elements[i]));
    }
}

/**
 * @brief Allocates and initializes a str_array.
 * @return A pointer to the new array, or NULL on failure.
 */
str_array *str_array_new();

/**
 * @brief Adds a string to an str_array.
 * @param arr The array to add to.
 * @param str The string to add.
 * @return true on success, false on failure.
 */
int str_array_add(str_array *arr, const char *str);

/**
 * @brief Frees all memory associated with an str_array.
 * @param arr The array to free.
 */
void str_array_free(str_array *arr);

/**
 * @brief Converts a str_array into a single, comma-separated string.
 * @param arr The array to convert.
 * @return A dynamically allocated string, or NULL on failure.
 */
char *str_array_to_string(const str_array *arr);

#endif // MCPKG_ARRAY_HELPER_H
