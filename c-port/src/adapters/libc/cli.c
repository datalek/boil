/*
 * cli.c - libc CLI adapter implementation
 */

#include "cli.h"
#include "../../domain/str.h"
#include <stdio.h>
#include <stdlib.h>

static void libc_cli_write(const char *s) {
    if (s) {
        fputs(s, stdout);
        fflush(stdout);
    }
}

static char *libc_cli_prompt(const char *question) {
    if (question) {
        fputs(question, stdout);
        fflush(stdout);
    }

    char buffer[1024];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return str_dup("");
    }

    return str_trim(buffer);
}

Cli libc_cli_make(void) {
    return (Cli){
        .write = libc_cli_write,
        .prompt = libc_cli_prompt
    };
}
