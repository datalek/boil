/*
 * cli.h - libc CLI adapter
 */

#ifndef BOIL_ADAPTERS_LIBC_CLI_H
#define BOIL_ADAPTERS_LIBC_CLI_H

#include "../../domain/cli.h"

/*
 * Create CLI adapter using stdin/stdout.
 */
Cli libc_cli_make(void);

#endif /* BOIL_ADAPTERS_LIBC_CLI_H */
