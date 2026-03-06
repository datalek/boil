/*
 * fetch.h - curl fetch adapter
 */

#ifndef BOIL_ADAPTERS_CURL_FETCH_H
#define BOIL_ADAPTERS_CURL_FETCH_H

#include "../../domain/fetch.h"

/*
 * Create curl fetch adapter.
 * Supports HTTP/HTTPS URLs and local files.
 */
Fetcher curl_fetcher_make(void);

#endif /* BOIL_ADAPTERS_CURL_FETCH_H */
