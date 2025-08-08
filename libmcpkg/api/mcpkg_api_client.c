#include "api/mcpkg_api_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Callback function to write received data into a buffer
static size_t write_to_buffer_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    api_response *mem = (api_response *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        // Out of memory
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

// Allocates and initializes the client in one step
ApiClient *api_client_new() {
    ApiClient *client = calloc(1, sizeof(ApiClient));
    if (!client) {
        return NULL;
    }
    if (api_client_init(client) != 0) {
        free(client);
        return NULL;
    }
    return client;
}

int api_client_init(ApiClient *client) {
    if (!client) {
        return 1;
    }
    client->curl = curl_easy_init();
    if (!client->curl) {
        return 1;
    }
    return 0;
}

void api_client_free(ApiClient *client) {
    if (client) {
        api_client_cleanup(client);
        free(client);
    }
}

void api_client_cleanup(ApiClient *client) {
    if (client && client->curl) {
        curl_easy_cleanup(client->curl);
    }
}

CURLcode api_client_get_with_callback(ApiClient *client, const char *url, struct curl_slist *headers, size_t (*write_cb)(void*, size_t, size_t, void*), void *user_data) {
    if (!client || !client->curl) {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(client->curl, CURLOPT_URL, url);
    curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, user_data);
    curl_easy_setopt(client->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(client->curl, CURLOPT_USERAGENT, "mcpkg/0.1.0");

    CURLcode res = curl_easy_perform(client->curl);
    return res;
}

cJSON *api_client_get(ApiClient *client, const char *url, struct curl_slist *headers) {
    if (!client || !client->curl) {
        return NULL;
    }

    api_response response = {0};
    cJSON *json = NULL;
    CURLcode res = api_client_get_with_callback(client, url, headers, write_to_buffer_callback, (void *)&response);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response.status_code);
        if (response.status_code == 200 && response.data) {
            json = cJSON_Parse(response.data);
            if (!json) {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr) {
                    fprintf(stderr, "Error before: %s\n", error_ptr);
                }
            }
        }
    }

    if (response.data) {
        free(response.data);
    }
    return json;
}
