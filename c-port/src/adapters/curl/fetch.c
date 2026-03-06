/*
 * fetch.c - libcurl fetch adapter implementation
 */

#include "fetch.h"
#include "../../domain/str.h"
#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * CURL write callback - appends data to Str buffer.
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    Str *buffer = (Str *)userp;
    str_append_n(buffer, (const char *)contents, total);
    return total;
}

/*
 * Fetch from HTTP/HTTPS URL using libcurl.
 */
static StringResult fetch_url(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return string_result_err("Failed to initialize CURL");
    }

    Str buffer = str_new();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "boil/1.0");

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        str_free(&buffer);
        curl_easy_cleanup(curl);

        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to fetch template from URL: %s", curl_easy_strerror(res));
        return string_result_err(msg);
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code < 200 || http_code >= 300) {
        str_free(&buffer);

        char msg[512];
        snprintf(msg, sizeof(msg), "HTTP %ld: Failed to fetch template", http_code);
        return string_result_err(msg);
    }

    char *result = str_dup(str_cstr(&buffer));
    str_free(&buffer);

    return string_result_ok(result);
}

/*
 * Read local file.
 */
static StringResult fetch_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to open file '%s': %s", path, strerror(errno));
        return string_result_err(msg);
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return string_result_err("Failed to determine file size");
    }

    char *content = malloc((size_t)size + 1);
    if (!content) {
        fclose(f);
        return string_result_err("Out of memory");
    }

    size_t read = fread(content, 1, (size_t)size, f);
    fclose(f);

    content[read] = '\0';
    return string_result_ok(content);
}

static StringResult curl_fetch(const char *path) {
    if (!path || !*path) {
        return string_result_err("Empty template path");
    }

    /* Check if URL */
    if (strncmp(path, "http://", 7) == 0 || strncmp(path, "https://", 8) == 0) {
        return fetch_url(path);
    }

    /* Read from local file */
    return fetch_file(path);
}

Fetcher curl_fetcher_make(void) {
    return (Fetcher){
        .fetch = curl_fetch
    };
}
