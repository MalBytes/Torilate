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

#define MAX_FLAG_COUNT     6
#define MAX_OPTION_COUNT   8

/* Types for CLI Arguments */
typedef enum {
    CMD_GET,
    CMD_POST,
} Command;

typedef struct {
    URI uri;
    Command cmd;
    bool flags[MAX_FLAG_COUNT];
    const char *options[MAX_OPTION_COUNT];
} CliArgsInfo;

/* Indices for fetching options form the CliArgsInfo.options array */
typedef enum {
    OPTION_INPUT_FILE,
    OPTION_OUTPUT_FILE,
} OptionsIndex;

/* Indices for fetching flags form the CliArgsInfo.flags array */
typedef enum {
    FLAG_RAW,
    FLAG_VERBOSE,
} FlagsIndex;


void get_help();
/* 
*   Parses command-line arguments and populates the provided CliArgsInfo structure.
*
*   Parameters:
*       argc      - argument count from main()
*       argv      - argument vector from main()
*       args_info - pointer to CliArgsInfo structure to populate with parsed data
*
*   Returns:
*       approprite ErrorCode based on the success or failure of parsing.
*/
ErrorCode parse_arguments(int argc, char *argv[], CliArgsInfo *args_info);

#endif