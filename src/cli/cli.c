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

/* Function Prototypes */
void cli_init();
int cmd_get_proc (int argc, char *argv[], arg_dstr_t res, void *ctx);


int parse_arguments(int argc, char *argv[], CliArgsInfo *args_info) {
    // TODO: Implement full argument parsing using argtable3

    /* Temporary argument parsing */
    cli_init();
    arg_cmd_register("get", cmd_get_proc, "send HTTP GET request", args_info);
    
    arg_dstr_t res = arg_dstr_create();
    if (argc == 1) {
        arg_make_get_help_msg(res);
        fprintf(stderr, "%s", arg_dstr_cstr(res));
        arg_dstr_destroy(res);
        arg_cmd_uninit();
        return INVALID_ARGS;
    }

    int rv = arg_cmd_dispatch(argv[1], argc, argv, res);
    printf("%s\n", arg_dstr_cstr(res));
    arg_dstr_destroy(res);
    arg_cmd_uninit();

    return rv;
}


void get_help() {
    arg_rex_t  *cmd_get    = arg_rex1(NULL,  NULL,  "get", NULL, ARG_REX_ICASE, "perform HTTP GET request");
    arg_str_t  *domain     = arg_str1(NULL, NULL, "<domain>", "domain to connect to");
    arg_str_t  *endpoint   = arg_str0(NULL, NULL, "<endpoint>", "endpoint to query [default: /]");
    arg_lit_t  *raw        = arg_lit0("r", "raw", "display raw HTTP response");
    arg_end_t  *end        = arg_end(20);

    void *argtable[] = {cmd_get, domain, endpoint, raw, end};
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

void cli_init() {
    arg_set_module_name(PROG_NAME);
    arg_set_module_version(VER_MAJOR, VER_MINOR, VER_PATCH, VER_TAG);
    arg_cmd_init();
}


int cmd_get_proc (int argc, char *argv[], arg_dstr_t res, void *ctx) {
    arg_rex_t  *cmd        = arg_rex1(NULL,  NULL,  "get", NULL, ARG_REX_ICASE, "perform HTTP GET request");
    arg_str_t  *domain     = arg_str1(NULL, NULL, "<domain>", "domain to connect to");
    arg_str_t  *endpoint   = arg_str0(NULL, NULL, "<endpoint>", "endpoint to query [default: /]");
    arg_lit_t  *raw        = arg_lit0("r", "raw", "display raw HTTP response");
    arg_end_t  *end        = arg_end(20);

    void *argtable[] = {cmd, domain, endpoint, raw, end};
    int exitcode = SUCCESS;
    if (arg_nullcheck(argtable) != 0) {
        fprintf(stderr, "failed to allocate argtable\n");
        exitcode = OUTOFMEMORY;
        goto exit;
    }

    int nerrors = arg_parse(argc, argv, argtable);
    if (arg_make_syntax_err_help_msg(res, "get", 0, nerrors, argtable, end, &exitcode)) {
        arg_dstr_catf(res, "For more details, use '%s help <command>'\n", PROG_NAME);  
        goto exit;
    }

    CliArgsInfo *args_info = (CliArgsInfo *)ctx;
    args_info->host = domain->sval[0];
    args_info->endpoint = endpoint->count > 0 ? endpoint->sval[0] : "/";
    args_info->port = 80; // Default HTTP port
    args_info->addr_type = DOMAIN;
    
    // Set flags
    char *raw_flag = raw->count > 0 ? "r" : "";
    snprintf((char *)args_info->flags, MAX_ARG_COUNT, "%s", raw_flag);
    
    arg_dstr_catf(res, "Preparing to send HTTP GET request to %s%s/%s via TOR...\n", args_info->host, args_info->endpoint, args_info->endpoint);

exit:
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    return exitcode;

}