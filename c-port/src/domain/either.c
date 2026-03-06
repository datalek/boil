/*
 * either.c - Either/Result type implementation
 */

#include "either.h"
#include <stdlib.h>
#include <string.h>

Result result_ok(void *value) {
    return (Result){.type = RESULT_OK, .value = value};
}

Result result_err(const char *error) {
    char *msg = error ? strdup(error) : strdup("Unknown error");
    return (Result){.type = RESULT_ERR, .error = msg};
}

void result_free_error(Result *r) {
    if (r->type == RESULT_ERR && r->error) {
        free(r->error);
        r->error = NULL;
    }
}

StringResult string_result_ok(char *value) {
    return (StringResult){.type = RESULT_OK, .value = value};
}

StringResult string_result_err(const char *error) {
    char *msg = error ? strdup(error) : strdup("Unknown error");
    return (StringResult){.type = RESULT_ERR, .error = msg};
}

void string_result_free(StringResult *r) {
    if (r->type == RESULT_OK && r->value) {
        free(r->value);
        r->value = NULL;
    } else if (r->type == RESULT_ERR && r->error) {
        free(r->error);
        r->error = NULL;
    }
}
