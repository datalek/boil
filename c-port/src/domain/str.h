/*
 * str.h - String utilities
 *
 * Provides:
 * - Dynamic string buffer (Str) for building strings
 * - String set for tracking unique variables
 * - Key-value map for variable substitution
 */

#ifndef BOIL_STR_H
#define BOIL_STR_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Dynamic string buffer.
 * Automatically grows as needed.
 */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Str;

/* Lifecycle */
Str str_new(void);
Str str_from(const char *s);
Str str_clone(const Str *s);
void str_free(Str *s);

/* Accessors */
static inline const char *str_cstr(const Str *s) { return s->data ? s->data : ""; }
static inline size_t str_len(const Str *s) { return s->len; }
static inline bool str_empty(const Str *s) { return s->len == 0; }

/* Modification */
void str_append(Str *s, const char *data);
void str_append_char(Str *s, char c);
void str_append_n(Str *s, const char *data, size_t n);
void str_clear(Str *s);

/* Utilities */
char *str_trim(const char *s);  /* Returns heap-allocated trimmed copy */
char *str_dup(const char *s);   /* Safe strdup */

/*
 * String set - stores unique strings.
 * Used for tracking template variables.
 */
typedef struct {
    char **items;
    size_t len;
    size_t cap;
} StrSet;

StrSet strset_new(void);
void strset_free(StrSet *set);
bool strset_add(StrSet *set, const char *s);  /* Returns true if added (was new) */
bool strset_contains(const StrSet *set, const char *s);
size_t strset_len(const StrSet *set);

/*
 * Key-value pair for variable substitution.
 */
typedef struct {
    char *key;
    char *value;
} KeyValue;

typedef struct {
    KeyValue *items;
    size_t len;
    size_t cap;
} KVMap;

KVMap kvmap_new(void);
void kvmap_free(KVMap *map);
void kvmap_set(KVMap *map, const char *key, const char *value);
const char *kvmap_get(const KVMap *map, const char *key);

/*
 * Replace all occurrences of {{key}} with value in string.
 * Returns heap-allocated result.
 */
char *str_substitute_vars(const char *content, const KVMap *values);

#endif /* BOIL_STR_H */
