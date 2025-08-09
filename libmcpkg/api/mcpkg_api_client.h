#ifndef MCPKG_API_CLIENT_H
#define MCPKG_API_CLIENT_H

#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stddef.h>

/* Single API response body (owned by caller after call returns) */
typedef struct {
    char  *data;
    size_t size;
    long   status_code;
} api_response;

typedef struct {
    CURL *curl;

    /* Last-response metadata (best-effort) */
    long last_status;
    long ratelimit_remaining;  /* X-RateLimit-Remaining */
    long ratelimit_reset;      /* X-RateLimit-Reset (epoch seconds or delta) */
} ApiClient;

/* Lifecycle */
ApiClient *api_client_new(void);
int        api_client_init(ApiClient *client);
void       api_client_free(ApiClient *client);
void       api_client_cleanup(ApiClient *client);

/* Config */
int  api_client_set_user_agent(ApiClient *client, const char *ua);
int  api_client_set_default_timeouts(ApiClient *client, long connect_ms, long total_ms);

/* Low-level GET (custom write callback) */
CURLcode api_client_get_with_callback(ApiClient *client, const char *url,
                                      struct curl_slist *headers,
                                      size_t (*write_cb)(void*, size_t, size_t, void*),
                                      void *user_data);

/* Mid-level GET that returns raw bytes (fills api_response; caller frees response->data) */
CURLcode api_client_get_raw(ApiClient *client, const char *url,
                            struct curl_slist *headers,
                            api_response *out);

/* High-level GET returning parsed JSON (caller cJSON_Delete()).
   Retries on 429/5xx with exponential backoff, honoring Retry-After when present. */
cJSON *api_client_get(ApiClient *client, const char *url, struct curl_slist *headers);

#endif /* MCPKG_API_CLIENT_H */
