/*
 * str.c - String utilities implementation
 */

#include "str.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAP 64

/* --- Str: Dynamic string buffer --- */

Str str_new(void) {
    return (Str){.data = NULL, .len = 0, .cap = 0};
}

Str str_from(const char *s) {
    if (!s) return str_new();

    size_t len = strlen(s);
    size_t cap = len + 1;
    char *data = malloc(cap);
    if (!data) return str_new();

    memcpy(data, s, len + 1);
    return (Str){.data = data, .len = len, .cap = cap};
}

Str str_clone(const Str *s) {
    if (!s || !s->data) return str_new();
    return str_from(s->data);
}

void str_free(Str *s) {
    if (s->data) {
        free(s->data);
        s->data = NULL;
    }
    s->len = 0;
    s->cap = 0;
}

static void str_grow(Str *s, size_t needed) {
    if (s->cap >= needed) return;

    size_t new_cap = s->cap ? s->cap * 2 : INITIAL_CAP;
    while (new_cap < needed) new_cap *= 2;

    char *new_data = realloc(s->data, new_cap);
    if (!new_data) return;

    s->data = new_data;
    s->cap = new_cap;
}

void str_append(Str *s, const char *data) {
    if (!data) return;
    str_append_n(s, data, strlen(data));
}

void str_append_char(Str *s, char c) {
    str_grow(s, s->len + 2);
    s->data[s->len++] = c;
    s->data[s->len] = '\0';
}

void str_append_n(Str *s, const char *data, size_t n) {
    if (!data || n == 0) return;

    str_grow(s, s->len + n + 1);
    memcpy(s->data + s->len, data, n);
    s->len += n;
    s->data[s->len] = '\0';
}

void str_clear(Str *s) {
    s->len = 0;
    if (s->data) s->data[0] = '\0';
}

/* --- Utilities --- */

char *str_trim(const char *s) {
    if (!s) return strdup("");

    while (isspace((unsigned char)*s)) s++;

    if (*s == '\0') return strdup("");

    const char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;

    size_t len = (size_t)(end - s + 1);
    char *result = malloc(len + 1);
    if (!result) return NULL;

    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

char *str_dup(const char *s) {
    return s ? strdup(s) : strdup("");
}

/* --- StrSet: Unique string set --- */

StrSet strset_new(void) {
    return (StrSet){.items = NULL, .len = 0, .cap = 0};
}

void strset_free(StrSet *set) {
    for (size_t i = 0; i < set->len; i++) {
        free(set->items[i]);
    }
    free(set->items);
    set->items = NULL;
    set->len = 0;
    set->cap = 0;
}

bool strset_contains(const StrSet *set, const char *s) {
    for (size_t i = 0; i < set->len; i++) {
        if (strcmp(set->items[i], s) == 0) return true;
    }
    return false;
}

bool strset_add(StrSet *set, const char *s) {
    if (strset_contains(set, s)) return false;

    if (set->len >= set->cap) {
        size_t new_cap = set->cap ? set->cap * 2 : 8;
        char **new_items = realloc(set->items, new_cap * sizeof(char *));
        if (!new_items) return false;
        set->items = new_items;
        set->cap = new_cap;
    }

    set->items[set->len++] = strdup(s);
    return true;
}

size_t strset_len(const StrSet *set) {
    return set->len;
}

/* --- KVMap: Key-value pairs --- */

KVMap kvmap_new(void) {
    return (KVMap){.items = NULL, .len = 0, .cap = 0};
}

void kvmap_free(KVMap *map) {
    for (size_t i = 0; i < map->len; i++) {
        free(map->items[i].key);
        free(map->items[i].value);
    }
    free(map->items);
    map->items = NULL;
    map->len = 0;
    map->cap = 0;
}

void kvmap_set(KVMap *map, const char *key, const char *value) {
    /* Check if key exists */
    for (size_t i = 0; i < map->len; i++) {
        if (strcmp(map->items[i].key, key) == 0) {
            free(map->items[i].value);
            map->items[i].value = strdup(value);
            return;
        }
    }

    /* Add new entry */
    if (map->len >= map->cap) {
        size_t new_cap = map->cap ? map->cap * 2 : 8;
        KeyValue *new_items = realloc(map->items, new_cap * sizeof(KeyValue));
        if (!new_items) return;
        map->items = new_items;
        map->cap = new_cap;
    }

    map->items[map->len].key = strdup(key);
    map->items[map->len].value = strdup(value);
    map->len++;
}

const char *kvmap_get(const KVMap *map, const char *key) {
    for (size_t i = 0; i < map->len; i++) {
        if (strcmp(map->items[i].key, key) == 0) {
            return map->items[i].value;
        }
    }
    return NULL;
}

/* --- Variable substitution --- */

/*
 * Find next {{variable}} pattern in string.
 * Returns pointer to opening {{ or NULL if not found.
 * Sets var_start to point after {{ and var_end to }}.
 */
static const char *find_var(const char *s, const char **var_start, const char **var_end) {
    const char *open = strstr(s, "{{");
    if (!open) return NULL;

    const char *close = strstr(open + 2, "}}");
    if (!close) return NULL;

    *var_start = open + 2;
    *var_end = close;
    return open;
}

char *str_substitute_vars(const char *content, const KVMap *values) {
    if (!content) return strdup("");

    Str result = str_new();
    const char *pos = content;

    while (*pos) {
        const char *var_start, *var_end;
        const char *open = find_var(pos, &var_start, &var_end);

        if (!open) {
            /* No more variables, append rest */
            str_append(&result, pos);
            break;
        }

        /* Append text before {{ */
        str_append_n(&result, pos, (size_t)(open - pos));

        /* Extract variable name (trimmed) */
        size_t var_len = (size_t)(var_end - var_start);
        char *var_name = malloc(var_len + 1);
        if (var_name) {
            memcpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';

            char *trimmed = str_trim(var_name);
            free(var_name);

            /* Look up value */
            const char *value = kvmap_get(values, trimmed);
            if (value) {
                str_append(&result, value);
            } else {
                /* Keep original if not found */
                str_append(&result, "{{");
                str_append(&result, trimmed);
                str_append(&result, "}}");
            }
            free(trimmed);
        }

        pos = var_end + 2;
    }

    char *out = result.data ? strdup(result.data) : strdup("");
    str_free(&result);
    return out;
}
