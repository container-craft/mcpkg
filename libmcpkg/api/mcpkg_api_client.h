#ifndef MCPKG_API_CLIENT_H
#define MCPKG_API_CLIENT_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <cjson/cJSON.h>

#include "mcpkg.h"
#include "mcpkg_export.h"
MCPKG_BEGIN_DECLS

#ifndef MCPKG_USER_AGENT
#define MCPKG_USER_AGENT "m_jimmer/mcpkg/" MCPKG_VERSION
#endif

typedef struct {
	char  *data;
	size_t size;
	long   status_code;
} ApiResponse;

typedef struct {
	CURL *curl;
	long last_status;
	long ratelimit_remaining;  /* X-RateLimit-Remaining */
	long ratelimit_reset;      /* X-RateLimit-Reset (epoch seconds or delta) */
	const char *user_agent;    /* UserAgent */
	const char *cache_root;
} ApiClient;

static size_t write_to_buffer_callback(void *contents,
                                       size_t size,
                                       size_t nmemb,
                                       void *userp)
{
	size_t realsize = size * nmemb;
	ApiResponse *mem = (ApiResponse *)userp;

	/* realloc to exact new size (+1 for NUL) */
	char *ptr = (char *)realloc(mem->data, mem->size + realsize + 1);
	if (!ptr)
		return 0; // signals error to libcurl

	mem->data = ptr;
	memcpy(mem->data + mem->size, contents, realsize);
	mem->size += realsize;
	mem->data[mem->size] = '\0';
	return realsize;
}


static void skip_spaces(const char **start,
                        const char *end)
{
	while (*start < end && (**start == ' ' || **start == '\t'))
		(*start)++;
}

static long parse_long_after_prefix(const char *buffer,
                                    size_t buffer_size,
                                    const char *prefix)
{
	size_t prefix_len = strlen(prefix);
	if (buffer_size < prefix_len)
		return -1;
	if (strncasecmp(buffer, prefix, prefix_len) != 0)
		return -1;

	const char *cursor = buffer + prefix_len;
	const char *buffer_end = buffer + buffer_size;

	/* Skip optional space(s) before the number */
	skip_spaces(&cursor, buffer_end);

	/* Parse signed/unsigned integer */
	char *parse_end = NULL;
	long parsed_value = strtol(cursor, &parse_end, 10);
	if (parse_end == cursor)
		return -1; /* no digits found */

	return parsed_value;
}


static size_t header_callback(char *buffer,
                              size_t size,
                              size_t nitems,
                              void *userdata)
{
	size_t realsize = size * nitems;
	ApiClient *client = (ApiClient *)userdata;

	long remaining;

	remaining = parse_long_after_prefix(buffer, realsize, "X-RateLimit-Remaining:");

	if (remaining < 0)
		remaining = parse_long_after_prefix(buffer, realsize, "x-ratelimit-remaining:");

	if (remaining >= 0)
		client->ratelimit_remaining = remaining;

	remaining = parse_long_after_prefix(buffer, realsize, "X-RateLimit-Reset:");
	if (remaining < 0)
		remaining = parse_long_after_prefix(buffer, realsize, "x-ratelimit-reset:");

	if (remaining >= 0)
		client->ratelimit_reset = remaining;

	// Retry-After can be absolute date too; we handle numeric seconds only
	remaining = parse_long_after_prefix(buffer, realsize, "Retry-After:");
	if (remaining < 0)
		remaining = parse_long_after_prefix(buffer, realsize, "retry-after:");

	if (remaining >= 0)
		client->ratelimit_reset = remaining;

	return realsize;
}

static void set_default_curl_options(CURL *curl)
{

	/* Follow redirects, accept gzip/deflate/br, HTTP/2 where possible */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	// TODO FIX
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); /* all supported */

#if defined(CURL_HTTP_VERSION_2TLS)
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
#endif

	// FIX
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,       30000L);

	// TODO
	// Fail on HTTP errors >= 400? We'll read status ourselves, so leave off
}

ApiClient *api_client_new(void);
MCPKG_ERROR_TYPE api_client_init(ApiClient *client);
void api_client_free(ApiClient *client);
void api_client_cleanup(ApiClient *client);


MCPKG_ERROR_TYPE api_client_set_user_agent(ApiClient *client,
                const char *ua);

MCPKG_ERROR_TYPE api_client_set_default_timeouts(ApiClient *client,
                long connect_ms,
                long total_ms);

/* Low-level GET (custom write callback) */
CURLcode api_client_get_with_callback(ApiClient *client,
                                      const char *url,
                                      struct curl_slist *headers,
                                      size_t (*write_cb)(void *, size_t, size_t, void *),
                                      void *user_data);

/* Mid-level GET that returns raw bytes (fills api_response; caller frees response->data) */
CURLcode api_client_get_raw(ApiClient *client, const char *url,
                            struct curl_slist *headers,
                            ApiResponse *out);

/* High-level GET returning parsed JSON (caller cJSON_Delete()).
   Retries on 429/5xx with exponential backoff, honoring Retry-After when present. */
cJSON *api_client_get(ApiClient *client, const char *url,
                      struct curl_slist *headers);

MCPKG_END_DECLS
#endif /* MCPKG_API_CLIENT_H */
