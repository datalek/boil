/*
 * cli.h - CLI port (interface)
 *
 * Defines the CLI interface that adapters must implement.
 */

#ifndef BOIL_DOMAIN_CLI_H
#define BOIL_DOMAIN_CLI_H

#include "either.h"
#include "str.h"

/*
 * Argument types enum.
 */
typedef enum {
    ARGS_TYPE_CREATE,
    ARGS_TYPE_REVERSE
} ArgsType;

/*
 * Create mode arguments.
 */
typedef struct {
    const char *project_name;
    const char *template_path;
} CreateArgs;

/*
 * Reverse mode arguments.
 */
typedef struct {
    const char *folder_path;
    const char *output_path;
} ReverseArgs;

/*
 * Parsed command line arguments.
 */
typedef struct {
    ArgsType type;
    union {
        CreateArgs create;
        ReverseArgs reverse;
    };
} InputArgs;

/*
 * CLI port - interface for CLI operations.
 * Adapters provide concrete implementations.
 */
typedef struct {
    void (*write)(const char *s);
    char *(*prompt)(const char *question);
} Cli;

/*
 * Parse command line arguments.
 *
 * Expected: boil <project-name> <template-path>
 *
 * Returns Result containing InputArgs* on success.
 */
Result cli_parse_args(int argc, char *argv[], const Cli *cli);

/*
 * Prompt user for variable values.
 *
 * Parameters:
 *   variables    - Set of variable names to prompt for
 *   project_name - Value for the 'name' variable (auto-filled)
 *   cli          - CLI adapter for I/O
 *
 * Returns Result containing KVMap* with all variable values.
 */
Result cli_prompt_variables(const StrSet *variables, const char *project_name, const Cli *cli);

#endif /* BOIL_DOMAIN_CLI_H */
