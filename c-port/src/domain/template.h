/*
 * template.h - Template parsing and file creation
 *
 * Defines Template types and operations.
 * File I/O is injected via the Fs port.
 */

#ifndef BOIL_DOMAIN_TEMPLATE_H
#define BOIL_DOMAIN_TEMPLATE_H

#include "either.h"
#include "str.h"
#include <stddef.h>

/*
 * Single file in a template.
 */
typedef struct {
    char *filename;
    char *content;
} TemplateFile;

/*
 * Parsed template containing files and variables.
 */
typedef struct {
    TemplateFile *files;
    size_t file_count;
    StrSet variables;
} Template;

void template_free(Template *t);

/*
 * Parse template content into structured Template.
 * Pure function - no I/O.
 */
Template template_parse(const char *content);

/*
 * Directory entry for readdir.
 */
typedef struct {
    char *name;
    bool is_dir;
} DirEntry;

/*
 * Directory listing result.
 */
typedef struct {
    DirEntry *entries;
    size_t count;
    char *error;
} DirResult;

void dir_result_free(DirResult *r);

/*
 * Filesystem port - interface for file operations.
 * Adapters provide concrete implementations.
 */
typedef struct {
    bool (*exists)(const char *path);
    bool (*is_dir)(const char *path);
    Result (*mkdir)(const char *path);
    Result (*write_file)(const char *path, const char *content);
    StringResult (*read_file)(const char *path);
    DirResult (*readdir)(const char *path);
    char *(*dirname)(const char *path);
    char *(*join)(const char *base, const char *name);
    char *(*realpath)(const char *path);
} Fs;

/*
 * Create project files from template.
 *
 * Parameters:
 *   root   - Project directory name
 *   t      - Parsed template
 *   values - Variable substitutions
 *   fs     - Filesystem adapter
 *
 * Returns Result with NULL on success, error message on failure.
 */
Result template_create_files(const char *root, const Template *t, const KVMap *values, const Fs *fs);

/*
 * Create template file from folder (reverse mode).
 *
 * Parameters:
 *   folder_path - Path to folder to convert to template
 *   output_path - Path where template file will be written
 *   fs          - Filesystem adapter
 *
 * Returns Result with NULL on success, error message on failure.
 */
Result template_create_from_folder(const char *folder_path, const char *output_path, const Fs *fs);

#endif /* BOIL_DOMAIN_TEMPLATE_H */
