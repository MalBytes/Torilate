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
#include "socks/socks4.h"
#include "argtable3/argtable3.h"

#define MAX_ARG_COUNT   5

/* Struct for CLI Arguments */
typedef struct {
    int port;
    const char *host;
    const char *endpoint;
    const char flags[MAX_ARG_COUNT];
    Socks4AddrType addr_type;
} CliArgsInfo;

void get_help();
int parse_arguments(int argc, char *argv[], CliArgsInfo *args_info);

#endif