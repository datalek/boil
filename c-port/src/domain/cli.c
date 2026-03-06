/*
 * cli.c - CLI domain logic implementation
 */

#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *USAGE =
    "Usage: boil <project-name> <template-path>\n"
    "\n"
    "Examples:\n"
    "  boil my-project ./templates/basic\n"
    "  boil my-app https://raw.githubusercontent.com/user/repo/main/template.hsfiles\n"
    "\n"
    "Reverse mode (create template from folder):\n"
    "  boil --reverse <folder-path> <output-template-path>\n";

static const char *REVERSE_USAGE =
    "Usage: boil --reverse <folder-path> <output-template-path>\n"
    "\n"
    "Examples:\n"
    "  boil --reverse ./my-project ./template.hsfiles\n"
    "  boil --reverse ./src/components ./components-template.hsfiles\n";

Result cli_parse_args(int argc, char *argv[], const Cli *cli) {
    /* Check for --reverse flag */
    int reverse_idx = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--reverse") == 0) {
            reverse_idx = i;
            break;
        }
    }

    if (reverse_idx >= 0) {
        /* Reverse mode: collect args excluding --reverse flag */
        int arg_count = 0;
        const char *args_without_flag[2] = {NULL, NULL};

        for (int i = 1; i < argc && arg_count < 2; i++) {
            if (i != reverse_idx) {
                args_without_flag[arg_count++] = argv[i];
            }
        }

        if (arg_count < 2) {
            cli->write(REVERSE_USAGE);
            return result_err("Insufficient arguments for reverse mode");
        }

        InputArgs *args = malloc(sizeof(InputArgs));
        if (!args) {
            return result_err("Out of memory");
        }

        args->type = ARGS_TYPE_REVERSE;
        args->reverse.folder_path = args_without_flag[0];
        args->reverse.output_path = args_without_flag[1];

        return result_ok(args);
    }

    /* Create mode */
    if (argc < 3) {
        cli->write(USAGE);
        return result_err("Insufficient arguments");
    }

    InputArgs *args = malloc(sizeof(InputArgs));
    if (!args) {
        return result_err("Out of memory");
    }

    args->type = ARGS_TYPE_CREATE;
    args->create.project_name = argv[1];
    args->create.template_path = argv[2];

    return result_ok(args);
}

Result cli_prompt_variables(const StrSet *variables, const char *project_name, const Cli *cli) {
    KVMap *values = malloc(sizeof(KVMap));
    if (!values) {
        return result_err("Out of memory");
    }

    *values = kvmap_new();

    /* Set project name as 'name' variable */
    kvmap_set(values, "name", project_name);

    /* Count variables that need prompting (excluding 'name') */
    size_t to_prompt = 0;
    for (size_t i = 0; i < variables->len; i++) {
        if (strcmp(variables->items[i], "name") != 0) {
            to_prompt++;
        }
    }

    if (to_prompt == 0) {
        return result_ok(values);
    }

    cli->write("Please provide values for the following variables:\n");

    for (size_t i = 0; i < variables->len; i++) {
        const char *var = variables->items[i];
        if (strcmp(var, "name") == 0) continue;

        /* Build prompt string */
        char prompt[256];
        snprintf(prompt, sizeof(prompt), "%s: ", var);

        char *value = cli->prompt(prompt);
        if (!value) {
            kvmap_free(values);
            free(values);
            return result_err("Failed to read input");
        }

        kvmap_set(values, var, value);
        free(value);
    }

    return result_ok(values);
}
