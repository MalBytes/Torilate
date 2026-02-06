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
int cmd_post_proc (int argc, char *argv[], arg_dstr_t res, void *ctx);

/* Function Implementations */
int parse_arguments(int argc, char *argv[], CliArgsInfo *args_info) {
    cli_init();
    arg_cmd_register("get", cmd_get_proc, "send HTTP GET request", args_info);
    arg_cmd_register("post", cmd_post_proc, "send HTTP POST request", args_info);
    
    arg_dstr_t res = arg_dstr_create();
    if (argc == 1) {
        arg_make_get_help_msg(res);
        fprintf(stderr, "%s", arg_dstr_cstr(res));
        arg_dstr_destroy(res);
        arg_cmd_uninit();
        return ERR_INVALID_ARGS;
    }

    int rv = arg_cmd_dispatch(argv[1], argc, argv, res);
    printf("%s: %s\n", PROG_NAME, arg_dstr_cstr(res));
    arg_dstr_destroy(res);
    arg_cmd_uninit();

    return rv;
}

void get_help() {
    arg_rex_t  *cmd        = arg_rex1(NULL,  NULL,  "get", NULL, ARG_REX_ICASE, "send a HTTP GET request");
    arg_str_t  *uri        = arg_str1(NULL, NULL, "<url>", "url to send request to");
    arg_lit_t  *raw        = arg_lit0("r", "raw", "display raw HTTP response");
    arg_end_t  *end        = arg_end(20);

    void *argtable[] = {cmd, uri, raw, end};
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
    arg_rex_t  *cmd         = arg_rex1(NULL,  NULL,  "get", NULL, ARG_REX_ICASE, "send a HTTP GET request");
    arg_str_t  *uri         = arg_str1(NULL, NULL, "<url>", "url to send request to");
    arg_str_t  *output_file = arg_str0("o", "output", "<output_file>", "output file to store the GET response");
    arg_lit_t  *raw         = arg_lit0("r", "raw", "display raw HTTP response");
    arg_end_t  *end         = arg_end(20);

    int exitcode = SUCCESS;
    void *argtable[] = {cmd, uri, output_file, raw, end};
    if (arg_nullcheck(argtable) != 0) {
        fprintf(stderr, "failed to allocate argtable");
        exitcode = ERR_OUTOFMEMORY;
        goto exit_get;
    }

    int nerrors = arg_parse(argc, argv, argtable);
    if (arg_make_syntax_err_help_msg(res, "get", 0, nerrors, argtable, end, &exitcode)) {
        arg_dstr_catf(res, "For more details, use '%s help <command>'", PROG_NAME);  
        goto exit_get;
    }

    // Parse URI and set values
    CliArgsInfo *args_info = (CliArgsInfo *)ctx;
    args_info->cmd = CMD_GET;
    int status = parse_uri(uri->sval[0], &args_info->uri);
    if (status != SUCCESS) {
        arg_dstr_catf(res, "Protocol '%s' is not supported", args_info->uri.host);
        exitcode = status;
        goto exit_get;
    }

    // Set output file
    if (output_file->count > 0) {
        args_info->output_file = output_file->sval[0];
    } else {
        args_info->output_file = NULL;
    }
    
    // Set flags
    char *raw_flag = raw->count > 0 ? "r" : "";
    snprintf((char *)args_info->flags, MAX_ARG_COUNT, "%s", raw_flag);
    
    arg_dstr_catf(res, "Preparing to send HTTP GET request to %s%s/%s via TOR...\n", args_info->uri.host, args_info->uri.endpoint, args_info->uri.endpoint);

exit_get:
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    return exitcode;
}

int cmd_post_proc (int argc, char *argv[], arg_dstr_t res, void *ctx) {
    arg_rex_t  *cmd         = arg_rex1(NULL,  NULL,  "post", NULL, ARG_REX_ICASE, "send a HTTP POST request");
    arg_str_t  *uri         = arg_str1(NULL, NULL, "<url>", "url to send request to");
    arg_str_t  *header      = arg_str0("t", "content-type", "<content-type>", "Content-Type header for the POST request");
    arg_str_t  *body        = arg_str0("b", "body", "<body>", "body of the POST request");
    arg_str_t  *input_file  = arg_str0("i", "input", "<input_file>", "input file for the POST request body");
    arg_str_t  *output_file = arg_str0("o", "output", "<output_file>", "output file to store the POST response");
    arg_lit_t  *raw         = arg_lit0("r", "raw", "display raw HTTP response");
    arg_end_t  *end         = arg_end(20);

    void *argtable[] = {cmd, uri, header, body, input_file, output_file, raw, end};
    int exitcode = SUCCESS;
    if (arg_nullcheck(argtable) != 0) {
        fprintf(stderr, "failed to allocate argtable");
        exitcode = ERR_OUTOFMEMORY;
        goto exit_post;
    }

    int nerrors = arg_parse(argc, argv, argtable);
    if (arg_make_syntax_err_help_msg(res, "post", 0, nerrors, argtable, end, &exitcode)) {
        arg_dstr_catf(res, "For more details, use '%s help <command>'", PROG_NAME);  
        goto exit_post;
    }

    // Parse URI and set values
    CliArgsInfo *args_info = (CliArgsInfo *)ctx;
    args_info->cmd = CMD_POST;
    int status = parse_uri(uri->sval[0], &args_info->uri);
    if (status != SUCCESS) {
        arg_dstr_catf(res, "Protocol '%s' is not supported", args_info->uri.host);
        exitcode = status;
        goto exit_post;
    }

    // Set header and body
    if (header->count > 0) {
        args_info->uri.header = header->sval[0];
    } else {
        args_info->uri.header = NULL;
    }
    if (body->count > 0) {
        args_info->uri.body = body->sval[0];
    } else {
        args_info->uri.body = NULL;
    }

    // Set input & output files
    if (input_file->count > 0) {
        args_info->input_file = input_file->sval[0];
    } else {
        args_info->input_file = NULL;
    }

    if (output_file->count > 0) {
        args_info->output_file = output_file->sval[0];
    } else {
        args_info->output_file = NULL;
    }   
    
    // Set flags
    char *raw_flag = raw->count > 0 ? "r" : "";
    snprintf((char *)args_info->flags, MAX_ARG_COUNT, "%s", raw_flag);
    
    arg_dstr_catf(res, "Preparing to send HTTP POST request to %s%s/%s via TOR...\n", args_info->uri.host, args_info->uri.endpoint, args_info->uri.endpoint);

exit_post:
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    return exitcode;
}