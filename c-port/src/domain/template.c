/*
 * template.c - Template parsing and file creation implementation
 */

#include "template.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_FILE_PREFIX "{-# START_FILE "
#define START_FILE_SUFFIX " #-}"

void template_free(Template *t) {
    for (size_t i = 0; i < t->file_count; i++) {
        free(t->files[i].filename);
        free(t->files[i].content);
    }
    free(t->files);
    t->files = NULL;
    t->file_count = 0;
    strset_free(&t->variables);
}

/*
 * Check if line matches START_FILE pattern.
 * Returns extracted filename (heap-allocated) or NULL.
 */
static char *parse_start_file(const char *line) {
    const char *prefix = START_FILE_PREFIX;
    const char *suffix = START_FILE_SUFFIX;
    size_t prefix_len = strlen(prefix);

    if (strncmp(line, prefix, prefix_len) != 0) return NULL;

    const char *end = strstr(line + prefix_len, suffix);
    if (!end) return NULL;

    size_t name_len = (size_t)(end - (line + prefix_len));
    char *name = malloc(name_len + 1);
    if (!name) return NULL;

    memcpy(name, line + prefix_len, name_len);
    name[name_len] = '\0';

    char *trimmed = str_trim(name);
    free(name);
    return trimmed;
}

/*
 * Extract all {{variable}} patterns from text.
 */
static void extract_variables(const char *text, StrSet *vars) {
    const char *pos = text;
    while (*pos) {
        const char *open = strstr(pos, "{{");
        if (!open) break;

        const char *close = strstr(open + 2, "}}");
        if (!close) break;

        size_t var_len = (size_t)(close - (open + 2));
        char *var = malloc(var_len + 1);
        if (var) {
            memcpy(var, open + 2, var_len);
            var[var_len] = '\0';

            char *trimmed = str_trim(var);
            free(var);

            if (trimmed && *trimmed) {
                strset_add(vars, trimmed);
            }
            free(trimmed);
        }

        pos = close + 2;
    }
}

Template template_parse(const char *content) {
    Template result = {
        .files = NULL,
        .file_count = 0,
        .variables = strset_new()
    };

    if (!content) return result;

    /* Parse line by line */
    const char *line_start = content;
    Str current_content = str_new();
    char *current_filename = NULL;

    size_t files_cap = 0;

    while (*line_start) {
        /* Find end of line */
        const char *line_end = strchr(line_start, '\n');
        size_t line_len = line_end ? (size_t)(line_end - line_start) : strlen(line_start);

        /* Copy line to null-terminated buffer */
        char *line = malloc(line_len + 1);
        if (!line) break;
        memcpy(line, line_start, line_len);
        line[line_len] = '\0';

        /* Check for START_FILE directive */
        char *new_filename = parse_start_file(line);
        if (new_filename) {
            /* Save previous file if any */
            if (current_filename) {
                if (result.file_count >= files_cap) {
                    files_cap = files_cap ? files_cap * 2 : 8;
                    TemplateFile *new_files = realloc(result.files, files_cap * sizeof(TemplateFile));
                    if (!new_files) {
                        free(line);
                        free(new_filename);
                        break;
                    }
                    result.files = new_files;
                }

                result.files[result.file_count].filename = current_filename;
                result.files[result.file_count].content = str_dup(str_cstr(&current_content));
                result.file_count++;
            }

            /* Start new file */
            current_filename = new_filename;
            str_clear(&current_content);
        } else {
            /* Append line to current content */
            str_append(&current_content, line);
            str_append_char(&current_content, '\n');
        }

        free(line);
        line_start = line_end ? line_end + 1 : line_start + line_len;
    }

    /* Save last file */
    if (current_filename) {
        if (result.file_count >= files_cap) {
            files_cap = files_cap ? files_cap * 2 : 8;
            TemplateFile *new_files = realloc(result.files, files_cap * sizeof(TemplateFile));
            if (new_files) {
                result.files = new_files;
            }
        }

        if (result.files) {
            result.files[result.file_count].filename = current_filename;
            result.files[result.file_count].content = str_dup(str_cstr(&current_content));
            result.file_count++;
        } else {
            free(current_filename);
        }
    }

    str_free(&current_content);

    /* Extract variables from all files */
    for (size_t i = 0; i < result.file_count; i++) {
        extract_variables(result.files[i].filename, &result.variables);
        extract_variables(result.files[i].content, &result.variables);
    }

    return result;
}

Result template_create_files(const char *root, const Template *t, const KVMap *values, const Fs *fs) {
    /* Check if root already exists */
    if (fs->exists(root)) {
        char msg[512];
        snprintf(msg, sizeof(msg), "The directory '%s' already exists", root);
        return result_err(msg);
    }

    /* Create root directory */
    Result mkdir_result = fs->mkdir(root);
    if (result_is_err(&mkdir_result)) {
        return mkdir_result;
    }
    result_free_error(&mkdir_result);

    /* Create each file */
    for (size_t i = 0; i < t->file_count; i++) {
        /* Substitute variables in filename and content */
        char *filename = str_substitute_vars(t->files[i].filename, values);
        char *content = str_substitute_vars(t->files[i].content, values);

        /* Build full path */
        char *file_path = fs->join(root, filename);

        /* Ensure parent directory exists */
        char *dir = fs->dirname(file_path);
        if (strcmp(dir, ".") != 0 && strcmp(dir, root) != 0) {
            Result dir_result = fs->mkdir(dir);
            if (result_is_err(&dir_result)) {
                free(filename);
                free(content);
                free(file_path);
                free(dir);
                return dir_result;
            }
            result_free_error(&dir_result);
        }
        free(dir);

        /* Remove trailing newline (added during parsing) */
        size_t len = strlen(content);
        if (len > 0 && content[len - 1] == '\n') {
            content[len - 1] = '\0';
        }

        /* Write file */
        Result write_result = fs->write_file(file_path, content);

        free(filename);
        free(content);
        free(file_path);

        if (result_is_err(&write_result)) {
            return write_result;
        }
        result_free_error(&write_result);
    }

    return result_ok(NULL);
}

void dir_result_free(DirResult *r) {
    if (r->entries) {
        for (size_t i = 0; i < r->count; i++) {
            free(r->entries[i].name);
        }
        free(r->entries);
        r->entries = NULL;
    }
    if (r->error) {
        free(r->error);
        r->error = NULL;
    }
    r->count = 0;
}

/*
 * Collected file for reverse mode.
 */
typedef struct {
    char *relative_path;
    char *content;
} CollectedFile;

/*
 * Collected files array.
 */
typedef struct {
    CollectedFile *files;
    size_t count;
    size_t capacity;
} CollectedFiles;

static CollectedFiles collected_files_new(void) {
    return (CollectedFiles){.files = NULL, .count = 0, .capacity = 0};
}

static void collected_files_free(CollectedFiles *cf) {
    if (cf->files) {
        for (size_t i = 0; i < cf->count; i++) {
            free(cf->files[i].relative_path);
            free(cf->files[i].content);
        }
        free(cf->files);
    }
    cf->files = NULL;
    cf->count = 0;
    cf->capacity = 0;
}

static bool collected_files_add(CollectedFiles *cf, const char *relative_path, const char *content) {
    if (cf->count >= cf->capacity) {
        size_t new_cap = cf->capacity ? cf->capacity * 2 : 16;
        CollectedFile *new_files = realloc(cf->files, new_cap * sizeof(CollectedFile));
        if (!new_files) return false;
        cf->files = new_files;
        cf->capacity = new_cap;
    }

    cf->files[cf->count].relative_path = strdup(relative_path);
    cf->files[cf->count].content = strdup(content);

    if (!cf->files[cf->count].relative_path || !cf->files[cf->count].content) {
        free(cf->files[cf->count].relative_path);
        free(cf->files[cf->count].content);
        return false;
    }

    cf->count++;
    return true;
}

/*
 * Recursively collect files from directory.
 */
static Result collect_files_recursive(const char *root_path, const char *current_path, 
                                       CollectedFiles *collected, const Fs *fs) {
    DirResult dir = fs->readdir(current_path);
    if (dir.error) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to read directory: %s", dir.error);
        dir_result_free(&dir);
        return result_err(msg);
    }

    for (size_t i = 0; i < dir.count; i++) {
        char *full_path = fs->join(current_path, dir.entries[i].name);
        if (!full_path) {
            dir_result_free(&dir);
            return result_err("Out of memory");
        }

        if (dir.entries[i].is_dir) {
            /* Recurse into subdirectory */
            Result sub_result = collect_files_recursive(root_path, full_path, collected, fs);
            free(full_path);
            if (result_is_err(&sub_result)) {
                dir_result_free(&dir);
                return sub_result;
            }
            result_free_error(&sub_result);
        } else {
            /* Read file content */
            StringResult content = fs->read_file(full_path);
            if (string_result_is_err(&content)) {
                char msg[512];
                snprintf(msg, sizeof(msg), "Failed to read file '%s': %s", full_path, content.error);
                string_result_free(&content);
                free(full_path);
                dir_result_free(&dir);
                return result_err(msg);
            }

            /* Calculate relative path */
            size_t root_len = strlen(root_path);
            const char *rel_start = full_path + root_len;
            /* Skip leading slash */
            if (*rel_start == '/') rel_start++;

            if (!collected_files_add(collected, rel_start, content.value)) {
                string_result_free(&content);
                free(full_path);
                dir_result_free(&dir);
                return result_err("Out of memory");
            }

            string_result_free(&content);
            free(full_path);
        }
    }

    dir_result_free(&dir);
    return result_ok(NULL);
}

/*
 * Generate template content from collected files.
 */
static char *generate_template_content(const CollectedFiles *collected) {
    Str content = str_new();

    for (size_t i = 0; i < collected->count; i++) {
        /* Add file header */
        str_append(&content, START_FILE_PREFIX);
        str_append(&content, collected->files[i].relative_path);
        str_append(&content, START_FILE_SUFFIX);
        str_append_char(&content, '\n');

        /* Add file content */
        str_append(&content, collected->files[i].content);

        /* Add newline between files (except last) */
        if (i < collected->count - 1) {
            str_append_char(&content, '\n');
        }
    }

    char *result = str_dup(str_cstr(&content));
    str_free(&content);
    return result;
}

Result template_create_from_folder(const char *folder_path, const char *output_path, const Fs *fs) {
    /* Resolve paths */
    char *resolved_folder = fs->realpath(folder_path);
    if (!resolved_folder) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Folder does not exist: %s", folder_path);
        return result_err(msg);
    }

    /* Check if folder is a directory */
    if (!fs->is_dir(resolved_folder)) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Path is not a directory: %s", resolved_folder);
        free(resolved_folder);
        return result_err(msg);
    }

    /* Check if output already exists */
    if (fs->exists(output_path)) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Output file already exists: %s", output_path);
        free(resolved_folder);
        return result_err(msg);
    }

    /* Collect all files */
    CollectedFiles collected = collected_files_new();
    Result collect_result = collect_files_recursive(resolved_folder, resolved_folder, &collected, fs);
    if (result_is_err(&collect_result)) {
        collected_files_free(&collected);
        free(resolved_folder);
        return collect_result;
    }
    result_free_error(&collect_result);

    if (collected.count == 0) {
        collected_files_free(&collected);
        free(resolved_folder);
        return result_err("No files found in folder");
    }

    /* Generate template content */
    char *template_content = generate_template_content(&collected);
    if (!template_content) {
        collected_files_free(&collected);
        free(resolved_folder);
        return result_err("Failed to generate template content");
    }

    /* Ensure output directory exists */
    char *output_dir = fs->dirname(output_path);
    if (output_dir && strcmp(output_dir, ".") != 0 && !fs->exists(output_dir)) {
        Result mkdir_result = fs->mkdir(output_dir);
        if (result_is_err(&mkdir_result)) {
            free(output_dir);
            free(template_content);
            collected_files_free(&collected);
            free(resolved_folder);
            return mkdir_result;
        }
        result_free_error(&mkdir_result);
    }
    free(output_dir);

    /* Write template file */
    Result write_result = fs->write_file(output_path, template_content);
    free(template_content);
    collected_files_free(&collected);
    free(resolved_folder);

    if (result_is_err(&write_result)) {
        return write_result;
    }

    return result_ok(NULL);
}
