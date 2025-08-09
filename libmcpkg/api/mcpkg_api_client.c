#include "api/mcpkg_api_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
/* ---------------- internal helpers ---------------- */

static size_t write_to_buffer_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    api_response *mem = (api_response *)userp;

    /* realloc to exact new size (+1 for NUL) */
    char *ptr = (char *)realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) return 0; /* signals error to libcurl */

    mem->data = ptr;
    memcpy(mem->data + mem->size, contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';
    return realsize;
}

/* --- helpers --- */
static void skip_spaces(const char **p, const char *end) {
    while (*p < end && (**p == ' ' || **p == '\t')) (*p)++;
}

static long parse_long_after_prefix(const char *buf, size_t n, const char *prefix) {
    size_t plen = strlen(prefix);
    if (n < plen) return -1;
    if (strncasecmp(buf, prefix, plen) != 0) return -1;

    const char *p = buf + plen;
    const char *end = buf + n;

    /* expect optional space(s) then the number */
    skip_spaces(&p, end);

    /* parse signed/unsigned integer */
    char *q = NULL;
    long v = strtol(p, &q, 10);
    if (q == p) return -1; /* no digits */
    return v;
}

static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t realsize = size * nitems;
    ApiClient *client = (ApiClient *)userdata;

    long v;

    v = parse_long_after_prefix(buffer, realsize, "X-RateLimit-Remaining:");
    if (v < 0) v = parse_long_after_prefix(buffer, realsize, "x-ratelimit-remaining:");
    if (v >= 0) client->ratelimit_remaining = v;

    v = parse_long_after_prefix(buffer, realsize, "X-RateLimit-Reset:");
    if (v < 0) v = parse_long_after_prefix(buffer, realsize, "x-ratelimit-reset:");
    if (v >= 0) client->ratelimit_reset = v;

    /* Retry-After can be absolute date too; we handle numeric seconds only */
    v = parse_long_after_prefix(buffer, realsize, "Retry-After:");
    if (v < 0) v = parse_long_after_prefix(buffer, realsize, "retry-after:");
    if (v >= 0) client->ratelimit_reset = v;

    return realsize;
}

static void set_default_curl_options(CURL *curl) {
    /* Follow redirects, accept gzip/deflate/br, HTTP/2 where possible */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); /* all supported */
#if defined(CURL_HTTP_VERSION_2TLS)
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
#endif
    /* Reasonable defaults; caller can override with api_client_set_default_timeouts */
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,       30000L);
    /* Fail on HTTP errors >= 400? We'll read status ourselves, so leave off. */
}

/* exponential backoff sleep (ms) */
static void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}


ApiClient *api_client_new(void) {
    ApiClient *client = (ApiClient *)calloc(1, sizeof(ApiClient));
    if (!client) return NULL;
    if (api_client_init(client) != 0) { free(client); return NULL; }
    return client;
}

int api_client_init(ApiClient *client) {
    if (!client) return 1;
    client->curl = curl_easy_init();
    if (!client->curl) return 1;

    set_default_curl_options(client->curl);

    /* Header callback to capture rate limit info */
    curl_easy_setopt(client->curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(client->curl, CURLOPT_HEADERDATA, (void *)client);

    /* Reasonable UA default; can be overridden */
    curl_easy_setopt(client->curl, CURLOPT_USERAGENT, "mcpkg/0.1.0");

    client->last_status = 0;
    client->ratelimit_remaining = -1;
    client->ratelimit_reset = -1;
    return 0;
}

int api_client_set_user_agent(ApiClient *client, const char *ua) {
    if (!client || !client->curl || !ua) return 1;
    return curl_easy_setopt(client->curl, CURLOPT_USERAGENT, ua) == CURLE_OK ? 0 : 1;
}

int api_client_set_default_timeouts(ApiClient *client, long connect_ms, long total_ms) {
    if (!client || !client->curl) return 1;
    if (connect_ms > 0) curl_easy_setopt(client->curl, CURLOPT_CONNECTTIMEOUT_MS, connect_ms);
    if (total_ms   > 0) curl_easy_setopt(client->curl, CURLOPT_TIMEOUT_MS,       total_ms);
    return 0;
}

void api_client_cleanup(ApiClient *client) {
    if (client && client->curl) {
        curl_easy_cleanup(client->curl);
        client->curl = NULL;
    }
}

void api_client_free(ApiClient *client) {
    if (!client) return;
    api_client_cleanup(client);
    free(client);
}

CURLcode api_client_get_with_callback(ApiClient *client, const char *url, struct curl_slist *headers,
                                      size_t (*write_cb)(void*, size_t, size_t, void*), void *user_data) {
    if (!client || !client->curl || !url) return CURLE_FAILED_INIT;

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

CURLcode api_client_get_raw(ApiClient *client, const char *url, struct curl_slist *headers, api_response *out) {
    if (!client || !client->curl || !url || !out) return CURLE_FAILED_INIT;

    memset(out, 0, sizeof(*out));
    CURLcode res = api_client_get_with_callback(client, url, headers, write_to_buffer_callback, (void *)out);

    if (res != CURLE_OK) {
        free(out->data); out->data = NULL; out->size = 0; out->status_code = 0;
        return res;
    }

    out->status_code = client->last_status;
    return res;
}

/* Retries: 0, 250ms, 500ms, 1000ms (max ~1.75s), unless Retry-After or ratelimit provides better guidance */
cJSON *api_client_get(ApiClient *client, const char *url, struct curl_slist *headers) {
    if (!client || !client->curl || !url) return NULL;

    const int max_attempts = 4;
    long backoff_ms = 250;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        api_response resp = {0};
        CURLcode rc = api_client_get_raw(client, url, headers, &resp);

        if (rc == CURLE_OK && resp.status_code == 200 && resp.data) {
            cJSON *json = cJSON_Parse(resp.data);
            free(resp.data);
            if (json) return json;

            /* JSON parse failed – treat as non-retryable */
            return NULL;
        }

        /* Decide whether to retry */
        int should_retry = 0;
        long wait_ms = backoff_ms;

        if (rc != CURLE_OK) {
            /* transient network error – retry */
            should_retry = 1;
        } else if (resp.status_code == 429) {
            /* Honor Retry-After or RateLimit-Reset if present */
            if (client->ratelimit_reset > 0) {
                /* If reset is seconds, convert to ms; if it's an epoch in the near future, compute delta */
                time_t now = time(NULL);
                long delta_s = client->ratelimit_reset > now ? (client->ratelimit_reset - now) : client->ratelimit_reset;
                if (delta_s < 0) delta_s = 1;
                wait_ms = (delta_s * 1000L);
            } else {
                wait_ms = 1000; /* default 1s */
            }
            should_retry = 1;
        } else if (resp.status_code >= 500 && resp.status_code <= 599) {
            should_retry = 1;
        }

        free(resp.data);

        if (!should_retry || attempt == max_attempts - 1) break;

        /* Backoff and retry */
        if (wait_ms < backoff_ms) wait_ms = backoff_ms;
        sleep_ms(wait_ms);
        backoff_ms <<= 1; /* exponential */
    }

    return NULL;
}
