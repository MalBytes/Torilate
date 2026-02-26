/*
    File: src/cli/cli.c
    Author: Trident Apollo  
    Date: 23-01-2026
    Reference:
        - Argtable3: https://www.argtable.org/docs/arg_getting_started.html
    Description:
        Implementation of command-line interface utilities for Torilate.
*/

#include "cli/cli.h"
#include "error/error.h"

// Represents a CLI subcommand with its handler and metadata
typedef struct {
    char name[50];
    arg_cmdfn handler;
    char description[200];
} SubCommand;

// Shared argument table elements used by all commands
typedef struct {
    arg_rex_t *cmd;
    arg_str_t *uri;
    arg_str_t *header;
    arg_str_t *output_file;
    arg_int_t *max_redirs;
    arg_lit_t *follow;
    arg_lit_t *raw;
    arg_lit_t *content_only;
    arg_lit_t *verbose;
    arg_end_t *end;
} CommonArgs;

// Complete argument table for GET command (only uses common args)
typedef struct {
    CommonArgs common;
} GetArgTable;

// Complete argument table for POST command (common + POST-specific args)
typedef struct {
    CommonArgs common;
    arg_str_t *body;
    arg_str_t *input_file;
} PostArgTable;

#define GET_ARGTABLE_ARRAY(args) (void*[]){ \
    args.common.cmd, args.common.uri, args.common.header, args.common.output_file, \
    args.common.max_redirs, args.common.follow, args.common.raw, \
    args.common.content_only, args.common.verbose, args.common.end \
}

#define POST_ARGTABLE_ARRAY(args) (void*[]){ \
    args.common.cmd, args.common.uri, args.common.header, args.body, \
    args.input_file, args.common.output_file, args.common.max_redirs, \
    args.common.follow, args.common.raw, args.common.content_only, \
    args.common.verbose, args.common.end \
}

#define GET_ARGTABLE_COUNT 10
#define POST_ARGTABLE_COUNT 12

// Function prototypes
int validate_command(char *cmd);
void cli_init(CliArgsInfo *args_info);
int cmd_get_proc (int argc, char *argv[], arg_dstr_t res, void *ctx);
int cmd_post_proc (int argc, char *argv[], arg_dstr_t res, void *ctx);
void init_common_args(CommonArgs *args, const char *cmd_name, const char *cmd_description);
GetArgTable get_args_table_get(void);
PostArgTable get_args_table_post(void);
void** get_common_args_help_table(int *count);
void** get_command_specific_args_table(const char *cmd_name, int *count);
void free_help_table(void **table, int count);

// Registry of available CLI subcommands
SubCommand sub_cmnds[] = {
    {"get", cmd_get_proc, "Send HTTP GET request"},
    {"post", cmd_post_proc, "Send HTTP POST request"},
};
int sub_cmnds_count = sizeof(sub_cmnds) / sizeof(SubCommand);

// Display dynamically-generated CLI help message
void get_help(void) {    
    printf("torilate —  A command-line utility that routes network traffic through the TOR network.\n\n");

    printf("Usage:\n");
    printf("  %s <command> <url> [options]\n\n", PROG_NAME);

    printf("Commands:\n");
    for (int i = 0; i < sub_cmnds_count; i++) {
        printf("  %-8s  %s\n", sub_cmnds[i].name, sub_cmnds[i].description);
    }
    printf("\n");

    printf("Common Options:\n");
    int common_count;
    void **common_table = get_common_args_help_table(&common_count);
    if (common_table) {
        arg_print_glossary_gnu_ex(stdout, common_table, 2, 35, 40, 150);
        free_help_table(common_table, common_count);
    }
    printf("\n");

    printf("Command-Specific Options:\n");
    for (int i = 0; i < sub_cmnds_count; i++) {
        int specific_count;
        void **specific_table = get_command_specific_args_table(sub_cmnds[i].name, &specific_count);
        
        if (specific_table && specific_count > 0) {
            printf("  %s:\n", sub_cmnds[i].name);
            arg_print_glossary_gnu_ex(stdout, specific_table, 4, 35, 40, 150);
            free_help_table(specific_table, specific_count);
        } else {
            printf("  %s:\n", sub_cmnds[i].name);
            printf("    (no additional options)\n");
        }
        printf("\n");
    }

    printf("Examples:\n");
    printf("  %s get example.com\n", PROG_NAME);
    printf("  %s get httpbin.org/redirect/3 -fl -v\n", PROG_NAME);
    printf("  %s post example.com -t application/json -b '{\"key\":\"value\"}'\n\n", PROG_NAME);
}

// Parse command-line arguments and populate CliArgsInfo
Error parse_arguments(int argc, char *argv[], CliArgsInfo *args_info) {
    if (argc < 2) {
        return ERR_NEW(ERR_NO_ARGS, "Use '%s help' for usage information", PROG_NAME);
    }

    if (validate_command(argv[1]) == -1) {
        return ERR_NEW(ERR_INVALID_COMMAND, "Invalid command '%s'. Use '%s help' for usage information.", argv[1], PROG_NAME);
    }

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

void cli_init(CliArgsInfo *args_info) {
    arg_set_module_name(PROG_NAME);
    arg_set_module_version(VER_MAJOR, VER_MINOR, VER_PATCH, VER_TAG);
    arg_cmd_init();

    for (int i=0; i < sub_cmnds_count; i++) {
        arg_cmd_register(sub_cmnds[i].name, sub_cmnds[i].handler, sub_cmnds[i].description, args_info);
    }

    memset(args_info, 0, sizeof(CliArgsInfo));
}
// Check if a command name is valid

int validate_command(char *cmd) {
    for (int i = 0; i < sub_cmnds_count; i++) {
        if (strcmp(cmd, sub_cmnds[i].name) == 0) {
            return i;
        }
// Initialize CLI subsystem and register commands
    }
    return -1;
}

// Initialize common argument table elements shared by all commands
void init_common_args(CommonArgs *args, const char *cmd_name, const char *cmd_description) {
    args->cmd          = arg_rex1(NULL, NULL, cmd_name, NULL, ARG_REX_ICASE, cmd_description);
    args->uri          = arg_str1(NULL, NULL, "<url>", "URL to send request to");
    args->header       = arg_strn("H", "header", "<header>", 0, 50, "HTTP header to include in the request");
    args->output_file  = arg_str0("o", "output", "<output_file>", "output file to store response");
    args->max_redirs   = arg_int0(NULL, "max-redirs", "<max_redirects>", "follow redirects up to the specified number of times");
    args->follow       = arg_lit0("fl", "follow", "follow redirects");
    args->raw          = arg_lit0("r", "raw", "display raw HTTP response");
    args->content_only = arg_lit0("c", "content-only", "display only the content of the HTTP response");
    args->verbose      = arg_lit0("v", "verbose", "display verbose output");
    args->end          = arg_end(20);
}

// Create and initialize argument table for GET command
GetArgTable get_args_table_get(void) {
    GetArgTable args;
    init_common_args(&args.common, "get", "send a HTTP GET request");
    return args;
}

// Create and initialize argument table for POST command
PostArgTable get_args_table_post(void) {
    PostArgTable args;
    init_common_args(&args.common, "post", "send a HTTP POST request");
    
    args.body       = arg_str0("b", "body", "<body>", "body of the POST request");
    args.input_file = arg_str0("i", "input", "<input_file>", 
                               "input file for the POST request body");
    
    return args;
}

// Create argtable for displaying common options in help
void** get_common_args_help_table(int *count) {
    CommonArgs args;
    init_common_args(&args, "dummy", "dummy");
    
    *count = 10;
    void **table = malloc((10 + 1) * sizeof(void*));
    if (!table) {
        void *temp_table[] = {args.cmd, args.uri, args.header, args.output_file,
                             args.max_redirs, args.follow, args.raw,
                             args.content_only, args.verbose, args.end};
        arg_freetable(temp_table, 10);
        *count = 0;
        return NULL;
    }
    
    // Store display elements, then end (ARG_TERMINATOR), then cmd for cleanup
    table[0] = args.uri;
    table[1] = args.header;
    table[2] = args.output_file;
    table[3] = args.max_redirs;
    table[4] = args.follow;
    table[5] = args.raw;
    table[6] = args.content_only;
    table[7] = args.verbose;
    table[8] = args.end;
    table[9] = args.cmd;
    table[10] = NULL;
    
    return table;
}

// Create argtable for command-specific options (POST-specific args)
void** get_command_specific_args_table(const char *cmd_name, int *count) {
    *count = 0;
    
    if (strcmp(cmd_name, "get") == 0) {
        return NULL;
    }
    else if (strcmp(cmd_name, "post") == 0) {
        PostArgTable args = get_args_table_post();
        
        *count = POST_ARGTABLE_COUNT;
        void **table = malloc((POST_ARGTABLE_COUNT + 1) * sizeof(void*));
        if (!table) {
            void *post_argtable[] = {args.common.cmd, args.common.uri, args.common.header,
                                     args.body, args.input_file, args.common.output_file,
                                     args.common.max_redirs, args.common.follow, args.common.raw,
                                     args.common.content_only, args.common.verbose, args.common.end};
            arg_freetable(post_argtable, POST_ARGTABLE_COUNT);
            *count = 0;
            return NULL;
        }
        
        table[0] = args.body;
        table[1] = args.input_file;
        table[2] = args.common.end;
        table[3] = args.common.cmd;
        table[4] = args.common.uri;
        table[5] = args.common.header;
        table[6] = args.common.output_file;
        table[7] = args.common.max_redirs;
        table[8] = args.common.follow;
        table[9] = args.common.raw;
        table[10] = args.common.content_only;
        table[11] = args.common.verbose;
        table[12] = NULL;
        
        return table;
// Free argtable allocated for help display
    }
    
    return NULL;
}

void free_help_table(void **table, int count) {
// Process GET command arguments
    if (table) {
        arg_freetable(table, count);
        free(table);
    }
}

int cmd_get_proc (int argc, char *argv[], arg_dstr_t res, void *ctx) {
    GetArgTable args = get_args_table_get();

    int exitcode = SUCCESS;
    void **argtable = GET_ARGTABLE_ARRAY(args);
    
    if (arg_nullcheck(argtable) != 0) {
        arg_dstr_cat(res, "failed to allocate argtable");
        exitcode = ERR_OUTOFMEMORY;
        goto exit_get;
    }

    // Populate CliArgsInfo with parsed values
    int nerrors = arg_parse(argc, argv, argtable);
    if (arg_make_syntax_err_help_msg(res, "get", 0, nerrors, argtable, args.common.end, &exitcode)) {
        arg_dstr_catf(res, "For more details, use '%s help <command>'", PROG_NAME);  
        goto exit_get;
    }

    CliArgsInfo *args_info = (CliArgsInfo *)ctx;
    args_info->cmd = CMD_GET;
    args_info->uri = args.common.uri->sval[0];
    
    Error err = get_schema(args.common.uri->sval[0], &args_info->schema);
    if (ERR_FAILED(err)) {
        arg_dstr_catf(res, err.message);
        exitcode = err.code;
        goto exit_get;
    }

    if (args.common.output_file->count > 0) {
        args_info->options[OPTION_OUTPUT_FILE] = args.common.output_file->sval[0];
    }
    
    if (args.common.header->count > 0) {
        int count = args.common.header->count;
        args_info->multi_options[MULTI_OPTION_HEADERS].count = 0;
        args_info->multi_options[MULTI_OPTION_HEADERS].values = NULL;

        char **values = malloc(sizeof(char*) * count);
        if (!values) {
            err = ERR_NEW(ERR_OUTOFMEMORY, "Failed to allocate memory for headers");
            arg_dstr_catf(res, err.message);
            exitcode = err.code;
            goto exit_get;
        }

        for (int i = 0; i < count; i++) {
            values[i] = strdup(args.common.header->sval[i]);
            if (!values[i]) {
                // cleanup previously allocated strings
                for (int j = 0; j < i; j++)
                    free(values[j]);
                free(values);

                err = ERR_NEW(ERR_OUTOFMEMORY, "Failed to allocate memory for header value");
                arg_dstr_catf(res, err.message);
                exitcode = err.code;
                goto exit_get;
            }
        }

        args_info->multi_options[MULTI_OPTION_HEADERS].values = (const char **)values;
        args_info->multi_options[MULTI_OPTION_HEADERS].count = count;
    }
    
    if (args.common.max_redirs->count > 0) {
        args_info->values[VAL_MAX_REDIRECTS] = args.common.max_redirs->ival[0];
    } else {
        args_info->values[VAL_MAX_REDIRECTS] = 50;
    }
    
    if (args.common.follow->count > 0) {
        args_info->flags[FLAG_FOLLOW] = true;
    }
    if (args.common.raw->count > 0) {
        args_info->flags[FLAG_RAW] = true;
    }
    if (args.common.content_only->count > 0) {
        args_info->flags[FLAG_CONTENT_ONLY] = true;
    }
    if (args.common.verbose->count > 0) {
        args_info->flags[FLAG_VERBOSE] = true;
    }

exit_get:
    arg_freetable(argtable, GET_ARGTABLE_COUNT);
    return exitcode;
}

// Process POST command arguments
int cmd_post_proc (int argc, char *argv[], arg_dstr_t res, void *ctx) {
    PostArgTable args = get_args_table_post();

    int exitcode = SUCCESS;
    void **argtable = POST_ARGTABLE_ARRAY(args);
    
    if (arg_nullcheck(argtable) != 0) {
        arg_dstr_cat(res, "failed to allocate argtable");
        exitcode = ERR_OUTOFMEMORY;
        goto exit_post;
    }
// Populate CliArgsInfo with parsed values
    
    int nerrors = arg_parse(argc, argv, argtable);
    if (arg_make_syntax_err_help_msg(res, "post", 0, nerrors, argtable, args.common.end, &exitcode)) {
        arg_dstr_catf(res, "For more details, use '%s help <command>'", PROG_NAME);  
        goto exit_post;
    }

    CliArgsInfo *args_info = (CliArgsInfo *)ctx;
    args_info->cmd = CMD_POST;
    args_info->uri = args.common.uri->sval[0];
    
    Error err = get_schema(args.common.uri->sval[0], &args_info->schema);
    if (ERR_FAILED(err)) {
        arg_dstr_catf(res, err.message);
        exitcode = err.code;
        goto exit_post;
    }

    if (args.body->count > 0) {
        args_info->options[OPTION_BODY] = args.body->sval[0];
    }

    if (args.input_file->count > 0) {
        args_info->options[OPTION_INPUT_FILE] = args.input_file->sval[0];
    }
    if (args.common.output_file->count > 0) {
        args_info->options[OPTION_OUTPUT_FILE] = args.common.output_file->sval[0];
    }

    if (args.common.header->count > 0) {
        int count = args.common.header->count;
        args_info->multi_options[MULTI_OPTION_HEADERS].count = 0;
        args_info->multi_options[MULTI_OPTION_HEADERS].values = NULL;

        char **values = malloc(sizeof(char*) * count);
        if (!values) {
            err = ERR_NEW(ERR_OUTOFMEMORY, "Failed to allocate memory for headers");
            arg_dstr_catf(res, err.message);
            exitcode = err.code;
            goto exit_post;
        }

        for (int i = 0; i < count; i++) {
            values[i] = strdup(args.common.header->sval[i]);
            if (!values[i]) {
                // cleanup previously allocated strings
                for (int j = 0; j < i; j++)
                    free(values[j]);
                free(values);

                err = ERR_NEW(ERR_OUTOFMEMORY, "Failed to allocate memory for header value");
                arg_dstr_catf(res, err.message);
                exitcode = err.code;
                goto exit_post;
            }
        }

        args_info->multi_options[MULTI_OPTION_HEADERS].values = (const char **)values;
        args_info->multi_options[MULTI_OPTION_HEADERS].count = count;
    }

    if (args.common.max_redirs->count > 0) {
        args_info->values[VAL_MAX_REDIRECTS] = args.common.max_redirs->ival[0];
    } else {
        args_info->values[VAL_MAX_REDIRECTS] = 50;
    }

    if (args.common.follow->count > 0) {
        args_info->flags[FLAG_FOLLOW] = true;
    }
    if (args.common.raw->count > 0) {
        args_info->flags[FLAG_RAW] = true;
    }
    if (args.common.content_only->count > 0) {
        args_info->flags[FLAG_CONTENT_ONLY] = true;
    }
    if (args.common.verbose->count > 0) {
        args_info->flags[FLAG_VERBOSE] = true;
    }

exit_post:
    arg_freetable(argtable, POST_ARGTABLE_COUNT);
    return exitcode;
}