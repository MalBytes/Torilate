/*
    File: src/cli/cli.h
    Author: Trident Apollo  
    Date: 23-01-2026
    Reference:
        - Argtable3: https://www.argtable.org/docs/arg_getting_started.html
    Description:
        Command-line interface definitions and utilities for Torilate.
*/

#ifndef TORILATE_CLI_H
#define TORILATE_CLI_H

#include "torilate.h"
#include "util/util.h"
#include "argtable3/argtable3.h"

#define MAX_ARG_COUNT   5

/* Types for CLI Arguments */
typedef enum {
    CMD_GET,
    CMD_POST,
} Command;
typedef struct {
    URI uri;
    Command cmd;
    const char *input_file;
    const char *output_file;
    const char flags[MAX_ARG_COUNT];
} CliArgsInfo;

void get_help();
ErrorCode parse_arguments(int argc, char *argv[], CliArgsInfo *args_info);

#endif