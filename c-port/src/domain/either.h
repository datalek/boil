/*
 * either.h - Either/Result type for explicit error handling
 *
 * Mirrors the TypeScript Either<L, R> type:
 * - Result is OK (success) or ERR (failure)
 * - Errors are values, not exceptions
 */

#ifndef BOIL_EITHER_H
#define BOIL_EITHER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    RESULT_OK,
    RESULT_ERR
} ResultType;

/*
 * Generic result type using void pointers.
 * Caller is responsible for memory management.
 */
typedef struct {
    ResultType type;
    union {
        void *value;  /* Success value */
        char *error;  /* Error message (heap-allocated) */
    };
} Result;

/* Constructors */
Result result_ok(void *value);
Result result_err(const char *error);

/* Predicates */
static inline bool result_is_ok(const Result *r) { return r->type == RESULT_OK; }
static inline bool result_is_err(const Result *r) { return r->type == RESULT_ERR; }

/* Accessors - caller must check type first */
static inline void *result_unwrap(const Result *r) { return r->value; }
static inline const char *result_error(const Result *r) { return r->error; }

/* Free error message if result is ERR */
void result_free_error(Result *r);

/*
 * String result - common case with owned string value.
 * Value must be heap-allocated; freed by caller.
 */
typedef struct {
    ResultType type;
    union {
        char *value;
        char *error;
    };
} StringResult;

StringResult string_result_ok(char *value);
StringResult string_result_err(const char *error);
void string_result_free(StringResult *r);

static inline bool string_result_is_ok(const StringResult *r) { return r->type == RESULT_OK; }
static inline bool string_result_is_err(const StringResult *r) { return r->type == RESULT_ERR; }

#endif /* BOIL_EITHER_H */
