/*
 * fs.c - libc filesystem adapter implementation
 */

#include "fs.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

static bool libc_fs_exists(const char *path) {
    return access(path, F_OK) == 0;
}

static bool libc_fs_is_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static Result libc_fs_mkdir(const char *path) {
    char *tmp = strdup(path);
    if (!tmp) return result_err("Out of memory");

    size_t len = strlen(tmp);

    /* Remove trailing slash */
    if (len > 0 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    /* Create each component */
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                char msg[512];
                snprintf(msg, sizeof(msg), "Failed to create directory '%s': %s", tmp, strerror(errno));
                free(tmp);
                return result_err(msg);
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to create directory '%s': %s", tmp, strerror(errno));
        free(tmp);
        return result_err(msg);
    }

    free(tmp);
    return result_ok(NULL);
}

static Result libc_fs_write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to create file '%s': %s", path, strerror(errno));
        return result_err(msg);
    }

    if (content && *content) {
        size_t len = strlen(content);
        size_t written = fwrite(content, 1, len, f);
        if (written != len) {
            fclose(f);
            return result_err("Failed to write file content");
        }
    }

    fclose(f);
    return result_ok(NULL);
}

static char *libc_fs_dirname(const char *path) {
    if (!path || !*path) return strdup(".");

    char *tmp = strdup(path);
    if (!tmp) return strdup(".");

    /* Remove trailing slashes */
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = '\0';
    }

    /* Find last slash */
    char *last = strrchr(tmp, '/');
    if (!last) {
        free(tmp);
        return strdup(".");
    }

    if (last == tmp) {
        /* Root directory */
        free(tmp);
        return strdup("/");
    }

    *last = '\0';
    char *result = strdup(tmp);
    free(tmp);
    return result ? result : strdup(".");
}

static char *libc_fs_join(const char *base, const char *name) {
    if (!base || !*base) return strdup(name ? name : "");
    if (!name || !*name) return strdup(base);

    size_t base_len = strlen(base);
    size_t name_len = strlen(name);
    bool need_slash = (base[base_len - 1] != '/') && (name[0] != '/');

    size_t total = base_len + name_len + (need_slash ? 1 : 0) + 1;
    char *result = malloc(total);
    if (!result) return NULL;

    strcpy(result, base);
    if (need_slash) strcat(result, "/");
    strcat(result, name);

    return result;
}

static DirResult dir_result_err_new(const char *error) {
    DirResult r;
    r.entries = NULL;
    r.count = 0;
    r.error = strdup(error);
    return r;
}

static DirResult dir_result_ok_new(DirEntry *entries, size_t count) {
    DirResult r;
    r.entries = entries;
    r.count = count;
    r.error = NULL;
    return r;
}

static StringResult libc_fs_read_file(const char *path) {
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
        return string_result_err("Failed to get file size");
    }

    char *content = malloc((size_t)size + 1);
    if (!content) {
        fclose(f);
        return string_result_err("Out of memory");
    }

    size_t read_size = fread(content, 1, (size_t)size, f);
    content[read_size] = '\0';
    fclose(f);

    return string_result_ok(content);
}

static DirResult libc_fs_readdir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to open directory '%s': %s", path, strerror(errno));
        return dir_result_err_new(msg);
    }

    /* Initial allocation */
    size_t capacity = 16;
    size_t count = 0;
    DirEntry *entries = malloc(capacity * sizeof(DirEntry));
    if (!entries) {
        closedir(dir);
        return dir_result_err_new("Out of memory");
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Grow array if needed */
        if (count >= capacity) {
            capacity *= 2;
            DirEntry *new_entries = realloc(entries, capacity * sizeof(DirEntry));
            if (!new_entries) {
                for (size_t i = 0; i < count; i++) {
                    free(entries[i].name);
                }
                free(entries);
                closedir(dir);
                return dir_result_err_new("Out of memory");
            }
            entries = new_entries;
        }

        entries[count].name = strdup(entry->d_name);
        if (!entries[count].name) {
            for (size_t i = 0; i < count; i++) {
                free(entries[i].name);
            }
            free(entries);
            closedir(dir);
            return dir_result_err_new("Out of memory");
        }

        /* Check if entry is directory */
        char *full_path = libc_fs_join(path, entry->d_name);
        if (full_path) {
            entries[count].is_dir = libc_fs_is_dir(full_path);
            free(full_path);
        } else {
            entries[count].is_dir = false;
        }

        count++;
    }

    closedir(dir);
    return dir_result_ok_new(entries, count);
}

static char *libc_fs_realpath(const char *path) {
    char resolved[PATH_MAX];
    if (realpath(path, resolved) == NULL) {
        return NULL;
    }
    return strdup(resolved);
}

Fs libc_fs_make(void) {
    return (Fs){
        .exists = libc_fs_exists,
        .is_dir = libc_fs_is_dir,
        .mkdir = libc_fs_mkdir,
        .write_file = libc_fs_write_file,
        .read_file = libc_fs_read_file,
        .readdir = libc_fs_readdir,
        .dirname = libc_fs_dirname,
        .join = libc_fs_join,
        .realpath = libc_fs_realpath
    };
}
