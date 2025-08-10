#include "mcpkg_api_client.h"
#include "mcpkg.h"

#include <stdio.h>
#include <time.h>

ApiClient *api_client_new(void) {
    ApiClient *client = (ApiClient *)calloc(1, sizeof(ApiClient));
    if (!client)
        return NULL;

    if (api_client_init(client) != MCPKG_ERROR_SUCCESS) {
        free(client);
        return NULL;
    }
    return client;
}

MCPKG_ERROR_TYPE api_client_init(ApiClient *client) {
    if (!client)
        return MCPKG_ERROR_OOM;

    const char *cache_root = getenv(ENV_MCPKG_CACHE);
    if (!cache_root) {
        cache_root = MCPKG_CACHE;
    }
    client->cache_root = cache_root;


    client->curl = curl_easy_init();
    if (!client->curl)
        return MCPKG_ERROR_OOM;

    set_default_curl_options(client->curl);

    curl_easy_setopt(client->curl,
                     CURLOPT_HEADERFUNCTION,
                     header_callback);

    curl_easy_setopt(client->curl,
                     CURLOPT_HEADERDATA,
                     (void *)client);

    const char *uagent = getenv("MCPKG_USER_AGENT");
    if (uagent && *uagent) {
        client->user_agent = uagent;
    } else {
        client->user_agent = MCPKG_USER_AGENT;
    }
    curl_easy_setopt(client->curl,
                     CURLOPT_USERAGENT,
                     client->user_agent);

    client->last_status = 0;
    client->ratelimit_remaining = -1;
    client->ratelimit_reset = -1;
    return MCPKG_ERROR_SUCCESS;
}

MCPKG_ERROR_TYPE api_client_set_user_agent(ApiClient *client,
                                           const char *ua)
{
    if (!client || !client->curl || !ua)
        return MCPKG_ERROR_OOM;

    // its already this
    if(strcmp(ua, client->user_agent) == 0 )
        return MCPKG_ERROR_SUCCESS;

    client->user_agent = ua;

    if(curl_easy_setopt(client->curl, CURLOPT_USERAGENT, client->user_agent) == CURLE_OK)
        return MCPKG_ERROR_SUCCESS;

    return MCPKG_ERROR_NETWORK;
}

MCPKG_ERROR_TYPE api_client_set_default_timeouts(ApiClient *client,
                                                 long connect_ms,
                                                 long total_ms)
{
    if (!client || !client->curl)
        return MCPKG_ERROR_OOM;

    if (connect_ms > 0)
        curl_easy_setopt(client->curl,
                         CURLOPT_CONNECTTIMEOUT_MS,
                         connect_ms);

    if (total_ms > 0)
        curl_easy_setopt(client->curl, CURLOPT_TIMEOUT_MS,       total_ms);

    return MCPKG_ERROR_SUCCESS;
}

void api_client_cleanup(ApiClient *client)
{
    if (client && client->curl) {
        curl_easy_cleanup(client->curl);
        client->curl = NULL;
    }
}

void api_client_free(ApiClient *client)
{
    if (!client)
        return;
    api_client_cleanup(client);
    // client->user_agent
    // free((char *)client->user_agent);
    free(client);
}

CURLcode api_client_get_with_callback(ApiClient *client,
                                      const char *url,
                                      struct curl_slist *headers,
                                      size_t (*write_cb)(void*, size_t, size_t, void*),
                                      void *user_data)
{
    if (!client || !client->curl || !url)
        return CURLE_FAILED_INIT;

    /* Reset last-response metadata */
    client->last_status = 0;
    client->ratelimit_remaining = -1;
    client->ratelimit_reset = -1;

    curl_easy_setopt(client->curl, CURLOPT_URL, url);
    curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, user_data);

    CURLcode res = curl_easy_perform(client->curl);
    if (res == CURLE_OK) {
        long code = 0;
        curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &code);
        client->last_status = code;
    }
    return res;
}

CURLcode api_client_get_raw(ApiClient *client,
                            const char *url,
                            struct curl_slist *headers,
                            ApiResponse *out)
{
    if (!client || !client->curl || !url || !out)
        return CURLE_FAILED_INIT;

    memset(out, 0, sizeof(*out));
    CURLcode res = api_client_get_with_callback(client,
                                                url,
                                                headers,
                                                write_to_buffer_callback,
                                                (void *)out);

    if (res != CURLE_OK) {
        free(out->data); out->data = NULL; out->size = 0; out->status_code = 0;
        return res;
    }

    out->status_code = client->last_status;
    return res;
}

/*
 * Retries: 0, 250ms, 500ms, 1000ms (max ~1.75s), unless Retry-After or ratelimit
 * provides better guidance
 */
cJSON *api_client_get(ApiClient *client,
                      const char *url,
                      struct curl_slist *headers)
{
    if (!client || !client->curl || !url)
        return NULL;

    const int max_attempts = 4;
    long backoff_ms = 250;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        ApiResponse resp = {0};
        CURLcode rc = api_client_get_raw(client, url, headers, &resp);

        if (rc == CURLE_OK && resp.status_code == 200 && resp.data) {
            cJSON *json = cJSON_Parse(resp.data);
            free(resp.data);
            if (json)
                return json;

            return NULL;
        }

        // retry ?
        int should_retry = 0;
        long wait_ms = backoff_ms;
        // FIXME
        // status code checking needs to be better.
        // It would be nice if the api's users could set custom ret code parseing out of this. Anyways that is for another time

        if (rc != CURLE_OK) {
            // transient network error – retry
            should_retry = 1;

        } else if (resp.status_code == 429) {
            // Honor Retry-After or RateLimit-Reset if present
            if (client->ratelimit_reset > 0) {

                // FIXME If reset is seconds, convert to ms; if it's an epoch in the near future, compute delta
                time_t now = time(NULL);

                long delta_s = client->ratelimit_reset > now ? (client->ratelimit_reset - now) : client->ratelimit_reset;
                if (delta_s < 0)
                    delta_s = 1;

                wait_ms = (delta_s * 1000L);
            } else {
                // FIXME make dynamic, default 1s ATM
                wait_ms = 1000;
            }
            should_retry = 1;

        } else if (resp.status_code >= 500 && resp.status_code <= 599) {
            should_retry = 1;
        }

        free(resp.data);

        if (!should_retry || attempt == max_attempts - 1)
            break;

        /* Backoff and retry */
        if (wait_ms < backoff_ms)
            wait_ms = backoff_ms;

        // this is dumb and will be handled other ways soon. . . .
        // sleep_ms(wait_ms);

        backoff_ms <<= 1; /* exponential */
    }

    return NULL;
}
