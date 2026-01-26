/*
    File: src/cli/cli.h
    Author: Trident Apollo  
    Date: 23-01-2026
    Reference:
        - Argtable3: https://www.argtable.org/docs/arg_getting_started.html
    Description:
        Implementation of command-line interface utilities for Torilate.
*/

#include "cli/cli.h"


int parse_arguments(int argc, char *argv[], CliArgsInfo *args_info) {
    // TODO: Implement full argument parsing using argtable3

    /* Temporary argument parsing */
    struct arg_rex  *cmd_get    = arg_rex1(NULL,  NULL,  "get", NULL, ARG_REX_ICASE, "perform HTTP GET request");
    struct arg_str  *domain     = arg_str1(NULL, NULL, "<domain>", "domain to connect to");
    struct arg_str  *endpoint   = arg_str0(NULL, NULL, "<endpoint>", "endpoint to query [default: /]");
    struct arg_end  *end1       = arg_end(20);

    void *argtable[] = {cmd_get, domain, endpoint, end1};
    if (arg_nullcheck(argtable) != 0) {
        fprintf(stderr, "Insufficient memory\n");
        return -1;
    }

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors > 0) {
        arg_print_errors(stderr, end1, PROG_NAME);
        arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return -1;
    }

    args_info->host = domain->sval[0];
    args_info->endpoint = endpoint->count > 0 ? endpoint->sval[0] : "/";
    args_info->port = 80; // Default HTTP port
    args_info->addr_type = DOMAIN;
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));

    return 0;
}

void get_help() {
    struct arg_rex  *cmd_get    = arg_rex1(NULL,  NULL,  "get", NULL, ARG_REX_ICASE, "perform HTTP GET request");
    struct arg_str  *domain     = arg_str1(NULL, NULL, "<domain>", "domain to connect to");
    struct arg_str  *endpoint   = arg_str0(NULL, NULL, "<endpoint>", "endpoint to query [default: /]");
    struct arg_end  *end1       = arg_end(20);

    void *argtable[] = {cmd_get, domain, endpoint, end1};
    if (arg_nullcheck(argtable) != 0) {
        fprintf(stderr, "Insufficient memory\n");
        return;
    }

    printf("Usage: %s", PROG_NAME);
    arg_print_syntax(stdout, argtable, "\n\n");
    printf("A command-line utility that routes network traffic through the TOR network \n\n");
    arg_print_glossary(stdout, argtable, "  %-25s %s\n");

    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
}