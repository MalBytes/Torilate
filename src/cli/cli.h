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
#define MAX_VALUE_COUNT    6
#define MAX_OPTION_COUNT   8

/* Types for CLI Arguments */
typedef enum {
    CMD_GET,
    CMD_POST,
} Command;

typedef struct {
    Command cmd;
    Schema schema;
    const char *uri;
    bool flags[MAX_FLAG_COUNT];             // Array to hold boolean flags
    int values[MAX_VALUE_COUNT];            // Array to hold integer values (e.g., max redirects)
    const char *options[MAX_OPTION_COUNT];  // Array to hold string options (e.g., output file)
} CliArgsInfo;

/* Indices for fetching options form the CliArgsInfo.options array */
typedef enum {
    OPTION_BODY,
    OPTION_HEADER,
    OPTION_INPUT_FILE,
    OPTION_OUTPUT_FILE,
} OptionsIndex;

/* Indices for fetching values form the CliArgsInfo.values array */
typedef enum {
    VAL_MAX_REDIRECTS,
} ValuesIndex;

/* Indices for fetching flags form the CliArgsInfo.flags array */
typedef enum {
    FLAG_RAW,
    FLAG_FOLLOW,
    FLAG_VERBOSE,
    FLAG_CONTENT_ONLY,
} FlagsIndex;


void get_help();
/* 
* Parses command-line arguments and populates the provided CliArgsInfo structure.
*
*  @param argc          argument count from main()
*  @param argv          argument vector from main()
*  @param args_info     pointer to CliArgsInfo structure to populate with parsed data
*
*  @returns            approprite ErrorCode based on the success or failure of parsing.
*/
Error parse_arguments(int argc, char *argv[], CliArgsInfo *args_info);

#endif