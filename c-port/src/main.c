/*
 * main.c - Boil entry point (composition root)
 *
 * Wires adapters to domain at startup.
 */

#include "domain/cli.h"
#include "domain/fetch.h"
#include "domain/template.h"
#include "adapters/libc/cli.h"
#include "adapters/libc/fs.h"
#include "adapters/curl/fetch.h"
#include <stdio.h>
#include <stdlib.h>

static void log_error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

/*
 * Handle create mode: template -> project
 */
static int handle_create(const CreateArgs *args, const Cli *cli, const Fetcher *fetcher, const Fs *fs) {
    /* Show project name */
    printf("Creating project: %s\n", args->project_name);

    /* Fetch template */
    printf("Fetching template from: %s\n", args->template_path);
    StringResult template_result = fetch_template(args->template_path, fetcher);
    if (string_result_is_err(&template_result)) {
        log_error(template_result.error);
        string_result_free(&template_result);
        return 1;
    }

    /* Parse template */
    printf("Parsing template...\n");
    Template tmpl = template_parse(template_result.value);
    string_result_free(&template_result);

    if (tmpl.file_count == 0) {
        log_error("No files found in template");
        template_free(&tmpl);
        return 1;
    }

    printf("Found %zu files and %zu variables\n", tmpl.file_count, strset_len(&tmpl.variables));

    /* Prompt for variables */
    Result vars_result = cli_prompt_variables(&tmpl.variables, args->project_name, cli);
    if (result_is_err(&vars_result)) {
        log_error(result_error(&vars_result));
        result_free_error(&vars_result);
        template_free(&tmpl);
        return 1;
    }

    KVMap *values = (KVMap *)result_unwrap(&vars_result);

    /* Create files */
    printf("Creating project directory: %s\n", args->project_name);
    Result create_result = template_create_files(args->project_name, &tmpl, values, fs);

    if (result_is_err(&create_result)) {
        log_error(result_error(&create_result));
        result_free_error(&create_result);
        kvmap_free(values);
        free(values);
        template_free(&tmpl);
        return 1;
    }

    printf("Project '%s' created successfully!\n", args->project_name);
    printf("Generated %zu files\n", tmpl.file_count);

    /* Cleanup */
    result_free_error(&create_result);
    kvmap_free(values);
    free(values);
    template_free(&tmpl);

    return 0;
}

/*
 * Handle reverse mode: folder -> template
 */
static int handle_reverse(const ReverseArgs *args, const Fs *fs) {
    printf("Creating template from folder: %s\n", args->folder_path);

    Result result = template_create_from_folder(args->folder_path, args->output_path, fs);
    if (result_is_err(&result)) {
        log_error(result_error(&result));
        result_free_error(&result);
        return 1;
    }

    printf("Template created: %s\n", args->output_path);
    result_free_error(&result);
    return 0;
}

int main(int argc, char *argv[]) {
    /* Create adapters (composition root) */
    Cli cli = libc_cli_make();
    Fetcher fetcher = curl_fetcher_make();
    Fs fs = libc_fs_make();

    /* Parse command line arguments */
    Result args_result = cli_parse_args(argc, argv, &cli);
    if (result_is_err(&args_result)) {
        log_error(result_error(&args_result));
        result_free_error(&args_result);
        return 1;
    }

    InputArgs *args = (InputArgs *)result_unwrap(&args_result);
    int exit_code;

    if (args->type == ARGS_TYPE_REVERSE) {
        exit_code = handle_reverse(&args->reverse, &fs);
    } else {
        exit_code = handle_create(&args->create, &cli, &fetcher, &fs);
    }

    free(args);
    return exit_code;
}
