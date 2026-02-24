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


/* ============================================================================
 * Data Structures
 * ============================================================================ */

/**
 * SubCommand - Represents a CLI subcommand with its handler and metadata
 */
typedef struct {
    char name[50];           // Command name (e.g., "get", "post")
    arg_cmdfn handler;       // Function to handle command execution
    char description[200];   // Brief description for help text
} SubCommand;

/**
 * CommonArgs - Shared argument table elements used by all commands
 * 
 * Contains argtable3 pointers for arguments common to GET and POST commands.
 * These are initialized once and reused to maintain consistency across commands.
 */
typedef struct {
    arg_rex_t *cmd;          // Command name matcher ("get" or "post")
    arg_str_t *uri;          // Required URL argument
    arg_str_t *output_file;  // Optional output file for response
    arg_int_t *max_redirs;   // Optional max redirect count
    arg_lit_t *follow;       // Flag: follow redirects
    arg_lit_t *raw;          // Flag: display raw HTTP response
    arg_lit_t *content_only; // Flag: display only response content
    arg_lit_t *verbose;      // Flag: verbose output
    arg_end_t *end;          // Error tracking sentinel
} CommonArgs;

/**
 * GetArgTable - Complete argument table for GET command
 * 
 * GET command only uses common arguments with no additional parameters.
 */
typedef struct {
    CommonArgs common;       // All GET arguments are common
} GetArgTable;

/**
 * PostArgTable - Complete argument table for POST command
 * 
 * POST command includes all common arguments plus POST-specific parameters
 * for request body, content type, and input file.
 */
typedef struct {
    CommonArgs common;       // Shared arguments
    arg_str_t *header;       // Optional Content-Type header
    arg_str_t *body;         // Optional request body string
    arg_str_t *input_file;   // Optional input file for body
} PostArgTable;

/* ============================================================================
 * Helper Macros
 * ============================================================================ */

/**
 * GET_ARGTABLE_ARRAY - Constructs void* array for GET command argtable
 * 
 * Expands struct fields into properly ordered argtable array for arg_parse().
 * Order: cmd, uri, output_file, max_redirs, follow, raw, content_only, verbose, end
 */
#define GET_ARGTABLE_ARRAY(args) (void*[]){ \
    args.common.cmd, args.common.uri, args.common.output_file, \
    args.common.max_redirs, args.common.follow, args.common.raw, \
    args.common.content_only, args.common.verbose, args.common.end \
}

/**
 * POST_ARGTABLE_ARRAY - Constructs void* array for POST command argtable
 * 
 * Expands struct fields into properly ordered argtable array for arg_parse().
 * Order: cmd, uri, header, body, input_file, output_file, max_redirs, 
 *        follow, raw, content_only, verbose, end
 */
#define POST_ARGTABLE_ARRAY(args) (void*[]){ \
    args.common.cmd, args.common.uri, args.header, args.body, \
    args.input_file, args.common.output_file, args.common.max_redirs, \
    args.common.follow, args.common.raw, args.common.content_only, \
    args.common.verbose, args.common.end \
}

/** Number of elements in command argtable */
#define GET_ARGTABLE_COUNT 9
#define POST_ARGTABLE_COUNT 12

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/* Command validation and CLI initialization */
int validate_command(char *cmd);
void cli_init(CliArgsInfo *args_info);

/* Command processors (registered with argtable3) */
int cmd_get_proc (int argc, char *argv[], arg_dstr_t res, void *ctx);
int cmd_post_proc (int argc, char *argv[], arg_dstr_t res, void *ctx);

/* Argument table initialization helpers */
void init_common_args(CommonArgs *args, const char *cmd_name, const char *cmd_description);
GetArgTable get_args_table_get(void);
PostArgTable get_args_table_post(void);

/* Dynamic help generation helpers */
void** get_common_args_help_table(int *count);
void** get_command_specific_args_table(const char *cmd_name, int *count);
void free_help_table(void **table, int count);

/* ============================================================================
 * Global Variables
 * ============================================================================ */

/**
 * sub_cmnds - Registry of available CLI subcommands
 * 
 * Each entry maps a command name to its handler function and description.
 * Add new commands here to extend CLI functionality.
 */
SubCommand sub_cmnds[] = {
    {"get", cmd_get_proc, "Send HTTP GET request"},
    {"post", cmd_post_proc, "Send HTTP POST request"},
};
int sub_cmnds_count = sizeof(sub_cmnds) / sizeof(SubCommand);

/* ============================================================================
 * Public API Implementation
 * ============================================================================ */

/**
 * get_help - Display comprehensive, dynamically-generated CLI help message
 * 
 * Generates and prints help text by introspecting registered commands and their
 * argument tables. Uses argtable3's arg_print_glossary_gnu() for consistent
 * formatting. Automatically adapts when new commands are added to sub_cmnds[].
 * 
 * Help structure:
 *   1. Usage syntax
 *   2. Program description
 *   3. Available commands (from sub_cmnds array)
 *   4. Common options (shared by all commands)
 *   5. Command-specific options (per command, dynamically generated)
 *   6. Usage examples
 */
void get_help(void) {    
    printf("torilate —  A command-line utility that routes network traffic through the TOR network.\n\n");

    printf("Usage:\n");
    printf("  %s <command> <url> [options]\n\n", PROG_NAME);

    /* -------------------- Commands -------------------- */
    printf("Commands:\n");
    for (int i = 0; i < sub_cmnds_count; i++) {
        printf("  %-8s  %s\n", sub_cmnds[i].name, sub_cmnds[i].description);
    }
    printf("\n");

    /* -------------------- Common Options -------------------- */
    printf("Common Options:\n");
    int common_count;
    void **common_table = get_common_args_help_table(&common_count);
    if (common_table) {
        arg_print_glossary_gnu_ex(stdout, common_table, 2, 35, 40, 150);
    }
    printf("\n");

    /* -------------------- Command-Specific Options -------------------- */
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

    /* -------------------- Examples -------------------- */
    printf("Examples:\n");
    printf("  %s get example.com\n", PROG_NAME);
    printf("  %s get httpbin.org/redirect/3 -fl -v\n", PROG_NAME);
    printf("  %s post example.com -t application/json -b '{\"key\":\"value\"}'\n\n", PROG_NAME);
    /* For future command specific help implementation*/
    // printf("Run 'torilate <command> --help' for command-specific options.\n");
    // printf("\n");

}

/**
 * parse_arguments - Parse command-line arguments and populate CliArgsInfo
 * 
 * Validates the command, initializes the argument parser, dispatches to the
 * appropriate command handler, and populates the provided CliArgsInfo structure.
 * 
 * @param argc       Argument count from main()
 * @param argv       Argument vector from main()
 * @param args_info  Pointer to structure to populate with parsed arguments
 * @return           Error structure (ERR_OK on success, error code on failure)
 */
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

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * cli_init - Initialize CLI subsystem and register commands
 * 
 * Sets up argtable3 module metadata, initializes command dispatcher, registers
 * all subcommands, and zeroes out the CliArgsInfo structure.
 * 
 * @param args_info  Pointer to CliArgsInfo structure to initialize
 */
void cli_init(CliArgsInfo *args_info) {
    // Set argtable3 module metadata
    arg_set_module_name(PROG_NAME);
    arg_set_module_version(VER_MAJOR, VER_MINOR, VER_PATCH, VER_TAG);
    arg_cmd_init();

    // Register all subcommands with their handlers
    for (int i=0; i < sub_cmnds_count; i++) {
        arg_cmd_register(sub_cmnds[i].name, sub_cmnds[i].handler, sub_cmnds[i].description, args_info);
    }

    // Zero out CliArgsInfo structure (sets all pointers to NULL, flags to false)
    memset(args_info, 0, sizeof(CliArgsInfo));
}

/**
 * validate_command - Check if a command name is valid
 * 
 * Searches the registered subcommands for a matching name.
 * 
 * @param cmd  Command name to validate
 * @return     Command index if found, -1 if invalid
 */
int validate_command(char *cmd) {
    for (int i = 0; i < sub_cmnds_count; i++) {
        if (strcmp(cmd, sub_cmnds[i].name) == 0) {
            return i;
        }
    }

    return -1;
}

/**
 * init_common_args - Initialize common argument table elements
 * 
 * Allocates and configures argtable3 argument descriptors shared by all commands.
 * This ensures consistent argument definitions across GET and POST.
 * 
 * @param args             Pointer to CommonArgs structure to populate
 * @param cmd_name         Command name for arg_rex pattern ("get" or "post")
 * @param cmd_description  Description for command matcher
 */
void init_common_args(CommonArgs *args, const char *cmd_name, const char *cmd_description) {
    args->cmd          = arg_rex1(NULL, NULL, cmd_name, NULL, ARG_REX_ICASE, cmd_description);
    args->uri          = arg_str1(NULL, NULL, "<url>", "URL to send request to");
    args->output_file  = arg_str0("o", "output", "<output_file>", "output file to store response");
    args->max_redirs   = arg_int0(NULL, "max-redirs", "<max_redirects>", "follow redirects up to the specified number of times");
    args->follow       = arg_lit0("fl", "follow", "follow redirects");
    args->raw          = arg_lit0("r", "raw", "display raw HTTP response");
    args->content_only = arg_lit0("c", "content-only", "display only the content of the HTTP response");
    args->verbose      = arg_lit0("v", "verbose", "display verbose output");
    args->end          = arg_end(20);
}

/**
 * get_args_table_get - Create and initialize argument table for GET command
 * 
 * Constructs a GetArgTable with all necessary arguments for parsing GET requests.
 * The returned structure contains named pointers to argtable3 elements that can
 * be accessed as args.common.uri, args.common.verbose, etc.
 * 
 * @return  Initialized GetArgTable structure
 */
GetArgTable get_args_table_get(void) {
    GetArgTable args;
    init_common_args(&args.common, "get", "send a HTTP GET request");
    return args;
}

/**
 * get_args_table_post - Create and initialize argument table for POST command
 * 
 * Constructs a PostArgTable with common arguments plus POST-specific parameters
 * for headers, body, and input files. Named access: args.common.uri, args.header,
 * args.body, args.input_file, etc.
 * 
 * @return  Initialized PostArgTable structure
 */
PostArgTable get_args_table_post(void) {
    PostArgTable args;
    init_common_args(&args.common, "post", "send a HTTP POST request");
    
    // Initialize POST-specific arguments
    args.header     = arg_str0("t", "content-type", "<content-type>", 
                               "Content-Type header for the POST request");
    args.body       = arg_str0("b", "body", "<body>", "body of the POST request");
    args.input_file = arg_str0("i", "input", "<input_file>", 
                               "input file for the POST request body");
    
    return args;
}

/**
 * get_common_args_help_table - Create argtable for displaying common options in help
 * 
 * Builds an argtable containing only the common arguments shared by all commands.
 * This is used by get_help() to display common options using arg_print_glossary.
 * Excludes the cmd matcher and end sentinel (not useful for help display).
 * 
 * @param count  Output parameter: number of elements in returned table
 * @return       Dynamically allocated argtable (must be freed with free_help_table)
 */
void** get_common_args_help_table(int *count) {
    // Allocate common args (excluding cmd and end)
    arg_str_t *uri          = arg_str1(NULL, NULL, "<url>", "URL to send request to");
    arg_str_t *output_file  = arg_str0("o", "output", "<output_file>", "output file to store response");
    arg_int_t *max_redirs   = arg_int0(NULL, "max-redirs", "<max_redirects>", 
                                       "follow redirects up to the specified number of times (default: 50)");
    arg_lit_t *follow       = arg_lit0("fl", "follow", "follow redirects");
    arg_lit_t *raw          = arg_lit0("r", "raw", "display raw HTTP response");
    arg_lit_t *content_only = arg_lit0("c", "content-only", "display only the content of the HTTP response");
    arg_lit_t *verbose      = arg_lit0("v", "verbose", "display verbose output");
    arg_end_t *end          = arg_end(1);
    
    *count = 8;
    void **table = malloc((*count + 1) * sizeof(void*));  // +1 for NULL terminator
    if (!table) {
        *count = 0;
        return NULL;
    }
    
    table[0] = uri;
    table[1] = output_file;
    table[2] = max_redirs;
    table[3] = follow;
    table[4] = raw;
    table[5] = content_only;
    table[6] = verbose;
    table[7] = end;
    table[8] = NULL;  // NULL terminator for arg_print_glossary
    
    // Check if any allocation failed
    if (arg_nullcheck(table) != 0) {
        arg_freetable(table, *count);
        free(table);
        *count = 0;
        return NULL;
    }
    
    return table;
}

/**
 * get_command_specific_args_table - Create argtable for command-specific options
 * 
 * Builds an argtable containing only the options unique to a specific command
 * (i.e., not shared with other commands). Used by get_help() to display
 * command-specific options separately from common options.
 * 
 * @param cmd_name  Command name ("get", "post", etc.)
 * @param count     Output parameter: number of elements in returned table
 * @return          Dynamically allocated argtable (must be freed with free_help_table)
 *                  Returns NULL if command has no specific options
 */
void** get_command_specific_args_table(const char *cmd_name, int *count) {
    *count = 0;
    
    if (strcmp(cmd_name, "get") == 0) {
        // GET has no command-specific options (all are common)
        return NULL;
    }
    else if (strcmp(cmd_name, "post") == 0) {
        // POST-specific arguments
        arg_str_t *header     = arg_str0("t", "content-type", "<content-type>", 
                                         "Content-Type header for the POST request");
        arg_str_t *body       = arg_str0("b", "body", "<body>", 
                                         "body of the POST request");
        arg_str_t *input_file = arg_str0("i", "input", "<input_file>", 
                                         "input file for the POST request body");
        arg_end_t *end        = arg_end(1);
        
        *count = 4;
        void **table = malloc((*count + 1) * sizeof(void*));  // +1 for NULL terminator
        if (!table) {
            *count = 0;
            return NULL;
        }
        
        table[0] = header;
        table[1] = body;
        table[2] = input_file;
        table[3] = end;
        table[4] = NULL;  // NULL terminator
        
        // Check if any allocation failed
        if (arg_nullcheck(table) != 0) {
            arg_freetable(table, *count);
            free(table);
            *count = 0;
            return NULL;
        }
        
        return table;
    }
    
    // Unknown command or no specific options
    return NULL;
}

/**
 * free_help_table - Free argtable allocated for help display
 * 
 * Properly deallocates argtable elements created by get_common_args_help_table
 * or get_command_specific_args_table. Uses arg_freetable to free argtable3
 * structures, then frees the table array itself.
 * 
 * @param table  Argtable to free
 * @param count  Number of elements in table
 */
void free_help_table(void **table, int count) {
    if (table) {
        arg_freetable(table, count);
        free(table);
    }
}

/* ============================================================================
 * Command Processors
 * ============================================================================ */

/**
 * cmd_get_proc - Process GET command arguments
 * 
 * Argtable3 command handler for HTTP GET requests. Parses command-line arguments,
 * validates the URL, and populates the CliArgsInfo context structure with parsed
 * values. Uses structured argtable for maintainable, named access to arguments.
 * 
 * @param argc  Argument count
 * @param argv  Argument vector
 * @param res   Result string for error messages
 * @param ctx   Context pointer (CliArgsInfo*)
 * @return      SUCCESS (0) on success, error code otherwise
 */
int cmd_get_proc (int argc, char *argv[], arg_dstr_t res, void *ctx) {
    // Initialize argument table with named struct fields
    GetArgTable args = get_args_table_get();

    int exitcode = SUCCESS;
    void **argtable = GET_ARGTABLE_ARRAY(args);
    
    // Validate argtable allocation
    if (arg_nullcheck(argtable) != 0) {
        arg_dstr_cat(res, "failed to allocate argtable");
        exitcode = ERR_OUTOFMEMORY;
        goto exit_get;
    }

    // Parse command-line arguments
    int nerrors = arg_parse(argc, argv, argtable);
    if (arg_make_syntax_err_help_msg(res, "get", 0, nerrors, argtable, args.common.end, &exitcode)) {
        arg_dstr_catf(res, "For more details, use '%s help <command>'", PROG_NAME);  
        goto exit_get;
    }

    // Populate CliArgsInfo structure with parsed values
    CliArgsInfo *args_info = (CliArgsInfo *)ctx;
    args_info->cmd = CMD_GET;
    args_info->uri = args.common.uri->sval[0];
    
    // Validate and extract URL schema
    Error err = get_schema(args.common.uri->sval[0], &args_info->schema);
    if (ERR_FAILED(err)) {
        arg_dstr_catf(res, err.message);
        exitcode = err.code;
        goto exit_get;
    }

    // Set optional output file
    if (args.common.output_file->count > 0) {
        args_info->options[OPTION_OUTPUT_FILE] = args.common.output_file->sval[0];
    }
    
    // Set max redirects (default: 50)
    if (args.common.max_redirs->count > 0) {
        args_info->values[VAL_MAX_REDIRECTS] = args.common.max_redirs->ival[0];
    } else {
        args_info->values[VAL_MAX_REDIRECTS] = 50;
    }
    
    // Set boolean flags
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

/**
 * cmd_post_proc - Process POST command arguments
 * 
 * Argtable3 command handler for HTTP POST requests. Parses command-line arguments
 * including POST-specific options (headers, body, input file), validates the URL, 
 * and populates the CliArgsInfo context structure with parsed values. Uses structured
 * argtable for maintainable, named access to arguments.
 * 
 * @param argc  Argument count
 * @param argv  Argument vector
 * @param res   Result string for error messages
 * @param ctx   Context pointer (CliArgsInfo*)
 * @return      SUCCESS (0) on success, error code otherwise
 */
int cmd_post_proc (int argc, char *argv[], arg_dstr_t res, void *ctx) {
    // Initialize argument table with named struct fields
    PostArgTable args = get_args_table_post();

    int exitcode = SUCCESS;
    void **argtable = POST_ARGTABLE_ARRAY(args);
    
    // Validate argtable allocation
    if (arg_nullcheck(argtable) != 0) {
        arg_dstr_cat(res, "failed to allocate argtable");
        exitcode = ERR_OUTOFMEMORY;
        goto exit_post;
    }

    // Parse command-line arguments
    int nerrors = arg_parse(argc, argv, argtable);
    if (arg_make_syntax_err_help_msg(res, "post", 0, nerrors, argtable, args.common.end, &exitcode)) {
        arg_dstr_catf(res, "For more details, use '%s help <command>'", PROG_NAME);  
        goto exit_post;
    }

    // Populate CliArgsInfo structure with parsed values
    CliArgsInfo *args_info = (CliArgsInfo *)ctx;
    args_info->cmd = CMD_POST;
    args_info->uri = args.common.uri->sval[0];
    
    // Validate and extract URL schema
    Error err = get_schema(args.common.uri->sval[0], &args_info->schema);
    if (ERR_FAILED(err)) {
        arg_dstr_catf(res, err.message);
        exitcode = err.code;
        goto exit_post;
    }

    // Set POST-specific options (header, body)
    if (args.header->count > 0) {
        args_info->options[OPTION_HEADER] = args.header->sval[0];
    }
    if (args.body->count > 0) {
        args_info->options[OPTION_BODY] = args.body->sval[0];
    }

    // Set input and output files
    if (args.input_file->count > 0) {
        args_info->options[OPTION_INPUT_FILE] = args.input_file->sval[0];
    }
    if (args.common.output_file->count > 0) {
        args_info->options[OPTION_OUTPUT_FILE] = args.common.output_file->sval[0];
    }

    // Set max redirects (default: 50)
    if (args.common.max_redirs->count > 0) {
        args_info->values[VAL_MAX_REDIRECTS] = args.common.max_redirs->ival[0];
    } else {
        args_info->values[VAL_MAX_REDIRECTS] = 50;
    }

    // Set boolean flags
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