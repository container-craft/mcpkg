#ifndef MCPKG_API_CLIENT_H
#define MCPKG_API_CLIENT_H

#include <curl/curl.h>
#include <cjson/cJSON.h>

// Struct to hold a single API response
typedef struct {
    char *data;
    size_t size;
    long status_code;
} api_response;

// Struct to hold the API client state
typedef struct {
    CURL *curl;
} ApiClient;

/**
 * @brief Allocates and initializes a new API client.
 * @return A pointer to the new ApiClient struct, or NULL on failure.
 */
ApiClient *api_client_new();

/**
 * @brief Initializes an existing API client.
 * @param client A pointer to the ApiClient struct.
 * @return 0 on success, non-zero on failure.
 */
int api_client_init(ApiClient *client);

/**
 * @brief Frees all memory associated with the API client.
 * @param client A pointer to the ApiClient struct.
 */
void api_client_free(ApiClient *client);

/**
 * @brief Cleans up the API client without freeing the client struct itself.
 * @param client A pointer to the ApiClient struct.
 */
void api_client_cleanup(ApiClient *client);

/**
 * @brief Makes a GET request with a custom write callback and user data.
 *
 * @param client A pointer to the ApiClient struct.
 * @param url The URL for the GET request.
 * @param headers An array of strings for custom headers (can be NULL).
 * @param write_cb A custom write callback function.
 * @param user_data A pointer to user data to be passed to the callback.
 * @return The CURLcode result of the transfer.
 */
CURLcode api_client_get_with_callback(ApiClient *client, const char *url,
                                      struct curl_slist *headers,
                                      size_t (*write_cb)(void*, size_t, size_t, void*),
                                      void *user_data);

/**
 * @brief Makes a GET request and parses the JSON response.
 *
 * The response body is parsed into a cJSON object. The caller is
 * responsible for freeing the cJSON object using cJSON_Delete().
 *
 * @param client A pointer to the ApiClient struct.
 * @param url The URL for the GET request.
 * @param headers An array of strings for custom headers (can be NULL).
 * @return A cJSON object on success, or NULL on failure.
 */
cJSON *api_client_get(ApiClient *client, const char *url, struct curl_slist *headers);

#endif // MCPKG_API_CLIENT_H
