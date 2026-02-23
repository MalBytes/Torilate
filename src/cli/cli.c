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
#include "error/error.h"


/* Data Structures */
typedef struct {
    char name[50];
    arg_cmdfn handler;
    char description[200];
} SubCommand;

/* Function Prototypes */
int validate_command(char *cmd);
void cli_init(CliArgsInfo *args_info);
int cmd_get_proc (int argc, char *argv[], arg_dstr_t res, void *ctx);
int cmd_post_proc (int argc, char *argv[], arg_dstr_t res, void *ctx);

/* Global Variables */
SubCommand sub_cmnds[] = {
    {"get", cmd_get_proc, "send HTTP GET request"},
    {"post", cmd_post_proc, "send HTTP POST request"},
};
int sub_cmnds_count = sizeof(sub_cmnds) / sizeof(SubCommand);

/* Function Implementations */
Error parse_arguments(int argc, char *argv[], CliArgsInfo *args_info) {
    if (argc < 2) {
        return ERR_NEW(ERR_NO_ARGS, "Use '%s help' for usage information", PROG_NAME);
    }

    if (validate_command(argv[1]) == -1) {
        return ERR_NEW(ERR_INVALID_COMMAND, "Invalid command '%s'. Use '%s help' for usage information.", argv[1], PROG_NAME);
    }

    // Initialize CLI and register commands
    cli_init(args_info);

    arg_dstr_t res = arg_dstr_create();
    if (argc == 1) {
        arg_make_get_help_msg(res);
        fprintf(stderr, "%s", arg_dstr_cstr(res));
        arg_dstr_destroy(res);
        arg_cmd_uninit();
        return ERR_NEW(ERR_INVALID_ARGS, "Failed to generate help message");
    }

    Error err;
    int rv = arg_cmd_dispatch(argv[1], argc, argv, res);
    if (rv != SUCCESS) {
        err = ERR_NEW(ERR_INVALID_ARGS, "Failed to parse command arguments: %s", arg_dstr_cstr(res));
    } else {
        err = ERR_OK();
    }
    arg_dstr_destroy(res);
    arg_cmd_uninit();

    return err;
}

void get_help(void) {    
    printf("\nUsage:\n");
    printf("  %s <command> <url> [options] [flags]\n\n", PROG_NAME);

    printf("Description:\n");
    printf("  A command-line utility that routes network traffic through the TOR network.\n\n");

    /* -------------------- Commands -------------------- */
    printf("Commands:\n");
    for (int i = 0; i < sub_cmnds_count; i++) {
        printf("  %-8s  %s\n", sub_cmnds[i].name, sub_cmnds[i].description);
    }
    printf("\n");

    /* -------------------- Common Options -------------------- */
    printf("Options:\n");
    printf("  -o,  --output <file>        output file to store response\n");
    printf("      --max-redirs <n>        follow redirects up to <n> times (default: 50)\n");
    printf("  -t,  --content-type <type>  Content-Type header for POST request\n");
    printf("  -b,  --body <body>          body of the POST request\n");
    printf("  -i,  --input <file>         input file for POST request body\n");
    printf("\n");

    /* -------------------- Flags -------------------- */
    printf("Flags:\n");
    printf("  -fl, --follow               follow HTTP redirects\n");
    printf("  -r,  --raw                  display raw HTTP response\n");
    printf("  -c,  --content-only         display only response content\n");
    printf("  -v,  --verbose              display verbose output\n");
    printf("\n");

    printf("Examples:\n");
    printf("  %s get example.com\n", PROG_NAME);
    printf("  %s get httpbin.org/redirect/3 -fv\n", PROG_NAME);
    printf("  %s post example.com -t application/json -b '{\"key\":\"value\"}'\n", PROG_NAME);
    printf("\n");
}

/* Helper Functions */
void cli_init(CliArgsInfo *args_info) {
    arg_set_module_name(PROG_NAME);
    arg_set_module_version(VER_MAJOR, VER_MINOR, VER_PATCH, VER_TAG);
    arg_cmd_init();

    // Register sub-commands
    for (int i=0; i < sub_cmnds_count; i++) {
        arg_cmd_register(sub_cmnds[i].name, sub_cmnds[i].handler, sub_cmnds[i].description, args_info);
    }

    // Initialize the CliArgsInfo structure to NULL/0 values
    memset(args_info, 0, sizeof(CliArgsInfo));
}

int validate_command(char *cmd) {
    for (int i = 0; i < sub_cmnds_count; i++) {
        if (strcmp(cmd, sub_cmnds[i].name) == 0) {
            return i;
        }
    }

    return -1;
}

int cmd_get_proc (int argc, char *argv[], arg_dstr_t res, void *ctx) {
    arg_rex_t  *cmd             = arg_rex1(NULL,  NULL,  "get", NULL, ARG_REX_ICASE, "send a HTTP GET request");
    arg_str_t  *uri             = arg_str1(NULL, NULL, "<url>", "url to send request to");
    arg_str_t  *output_file     = arg_str0("o", "output", "<output_file>", "output file to store the GET response");
    arg_int_t  *max_redirs      = arg_int0(NULL, "max-redirs", "<max_redirects>", "follow redirects up to the specified number of times");
    arg_lit_t  *follow          = arg_lit0("fl", "follow", "follow redirects");
    arg_lit_t  *raw             = arg_lit0("r", "raw", "display raw HTTP response");
    arg_lit_t  *content_only    = arg_lit0("c", "content-only", "display only the content of the HTTP response");
    arg_lit_t  *verbose         = arg_lit0("v", "verbose", "display verbose output");
    arg_end_t  *end             = arg_end(20);

    int exitcode = SUCCESS;
    void *argtable[] = {cmd, uri, output_file, max_redirs, follow, raw, content_only, verbose, end};
    if (arg_nullcheck(argtable) != 0) {
        arg_dstr_cat(res, "failed to allocate argtable");
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
    args_info->uri = uri->sval[0];
    Error err = get_schema(uri->sval[0], &args_info->schema);
    if (ERR_FAILED(err)) {
        arg_dstr_catf(res, err.message);
        exitcode = err.code;
        goto exit_get;
    }

    // Set opptions
    if (output_file->count > 0) {
        args_info->options[OPTION_OUTPUT_FILE] = output_file->sval[0];
    }
    if (content_only->count > 0) {
        args_info->flags[FLAG_CONTENT_ONLY] = true;
    }

    // Set values
    if (max_redirs->count > 0) {
        args_info->values[VAL_MAX_REDIRECTS] = max_redirs->ival[0];
    } else {
        args_info->values[VAL_MAX_REDIRECTS] = 50; // Default to 50
    }
    
    // Set flags
    if (follow->count > 0) {
        args_info->flags[FLAG_FOLLOW] = true;
    } if (raw->count > 0) {
        args_info->flags[FLAG_RAW] = true;
    } if (verbose->count > 0) {
        args_info->flags[FLAG_VERBOSE] = true;
    }

exit_get:
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    return exitcode;
}

int cmd_post_proc (int argc, char *argv[], arg_dstr_t res, void *ctx) {
    arg_rex_t  *cmd             = arg_rex1(NULL,  NULL,  "post", NULL, ARG_REX_ICASE, "send a HTTP POST request");
    arg_str_t  *uri             = arg_str1(NULL, NULL, "<url>", "url to send request to");
    arg_str_t  *header          = arg_str0("t", "content-type", "<content-type>", "Content-Type header for the POST request");
    arg_str_t  *body            = arg_str0("b", "body", "<body>", "body of the POST request");
    arg_str_t  *input_file      = arg_str0("i", "input", "<input_file>", "input file for the POST request body");
    arg_str_t  *output_file     = arg_str0("o", "output", "<output_file>", "output file to store the POST response");
    arg_int_t  *max_redirs      = arg_int0(NULL, "max-redirs", "<max_redirects>", "follow redirects up to the specified number of times");
    arg_lit_t  *follow          = arg_lit0("fl", "follow", "follow redirects");
    arg_lit_t  *raw             = arg_lit0("r", "raw", "display raw HTTP response");
    arg_lit_t  *content_only    = arg_lit0("c", "content-only", "display only the content of the HTTP response");
    arg_lit_t  *verbose         = arg_lit0("v", "verbose", "display verbose output");
    arg_end_t  *end             = arg_end(20);

    void *argtable[] = {cmd, uri, header, body, input_file, output_file, max_redirs, follow, raw, content_only, verbose, end};
    int exitcode = SUCCESS;
    if (arg_nullcheck(argtable) != 0) {
        arg_dstr_cat(res, "failed to allocate argtable");
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
    args_info->uri = uri->sval[0];
    Error err = get_schema(uri->sval[0], &args_info->schema);
    if (ERR_FAILED(err)) {
        arg_dstr_catf(res, err.message);
        exitcode = err.code;
        goto exit_post;
    }

    // Set header and body
    if (header->count > 0) {
        args_info->options[OPTION_HEADER] = header->sval[0];
    } if (body->count > 0) {
        args_info->options[OPTION_BODY] = body->sval[0];
    }

    // Set input & output files
    if (input_file->count > 0) {
        args_info->options[OPTION_INPUT_FILE] = input_file->sval[0];
    } if (output_file->count > 0) {
        args_info->options[OPTION_OUTPUT_FILE] = output_file->sval[0];
    }

    // Set values
    if (max_redirs->count > 0) {
        args_info->values[VAL_MAX_REDIRECTS] = max_redirs->ival[0];
    } else {
        args_info->values[VAL_MAX_REDIRECTS] = 50; // Default to 50
    }

    // Set flags
    if (follow->count > 0) {
        args_info->flags[FLAG_FOLLOW] = true;
    } if (raw->count > 0) {
        args_info->flags[FLAG_RAW] = true;
    } if (content_only->count > 0) {
        args_info->flags[FLAG_CONTENT_ONLY] = true;
    } if (verbose->count > 0) {
        args_info->flags[FLAG_VERBOSE] = true;
    }

exit_post:
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    return exitcode;
}