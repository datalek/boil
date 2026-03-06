/*
 * fetch.h - Template fetching port
 *
 * Defines the interface for fetching templates.
 * Adapters provide implementations for HTTP, local files, etc.
 */

#ifndef BOIL_DOMAIN_FETCH_H
#define BOIL_DOMAIN_FETCH_H

#include "either.h"

/*
 * Fetch port - interface for template fetching.
 * Adapters provide concrete implementations.
 */
typedef struct {
    StringResult (*fetch)(const char *path);
} Fetcher;

/*
 * Fetch template using the provided fetcher.
 */
static inline StringResult fetch_template(const char *path, const Fetcher *fetcher) {
    return fetcher->fetch(path);
}

#endif /* BOIL_DOMAIN_FETCH_H */
