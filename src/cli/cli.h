/*
    File: src/cli/cli.h
    Author: Trident Apollo  
    Date: 23-01-2026
    Reference:
        - Argtable3: https://www.argtable.org/docs/arg_getting_started.html
    Description:
        Command-line interface definitions and utilities for Torilate.
        Provides parsing and validation for HTTP GET/POST commands with
        support for redirects, output files, and various display modes.
*/

#ifndef TORILATE_CLI_H
#define TORILATE_CLI_H

#include "torilate.h"
#include "util/util.h"
#include "argtable3/argtable3.h"

/* ============================================================================
 * Constants
 * ============================================================================ */

/** Maximum number of boolean flags in CliArgsInfo */
#define MAX_FLAG_COUNT     6

/** Maximum number of integer values in CliArgsInfo */
#define MAX_VALUE_COUNT    6

/** Maximum number of string options in CliArgsInfo */
#define MAX_OPTION_COUNT   8

/** Maximum number of multi-value options in CliArgsInfo */
#define MAX_MULTI_OPTION_COUNT   6

/* ============================================================================
 * Enumerations
 * ============================================================================ */

/**
 * Command - Supported HTTP methods
 * 
 * Identifies which HTTP command is being executed.
 */
typedef enum {
    CMD_GET,   // HTTP GET request
    CMD_POST,  // HTTP POST request
} Command;

/**
 * OptionsIndex - Indices for accessing string options in CliArgsInfo.options[]
 * 
 * Use these enum values to index into the options array for type-safe access
 * to parsed command-line string parameters.
 */
typedef enum {
    OPTION_BODY,         // POST request body content
    OPTION_INPUT_FILE,   // Input file path for POST body
    OPTION_OUTPUT_FILE,  // Output file path for response storage
} OptionsIndex;

/**
 * ValuesIndex - Indices for accessing integer values in CliArgsInfo.values[]
 * 
 * Use these enum values to index into the values array for type-safe access
 * to parsed command-line integer parameters.
 */
typedef enum {
    VAL_MAX_REDIRECTS,  // Maximum number of HTTP redirects to follow
} ValuesIndex;

/**
 * FlagsIndex - Indices for accessing boolean flags in CliArgsInfo.flags[]
 * 
 * Use these enum values to index into the flags array for type-safe access
 * to parsed command-line boolean flags.
 */
typedef enum {
    FLAG_RAW,           // Display raw HTTP response (headers + body)
    FLAG_FOLLOW,        // Follow HTTP redirect responses
    FLAG_VERBOSE,       // Display verbose diagnostic output
    FLAG_CONTENT_ONLY,  // Display only response body (no headers)
} FlagsIndex;

/**
 * MultiOptionsIndex - Indices for accessing multi-value options in CliArgsInfo.multi_options[]
 * 
 * Use these enum values to index into the multi_options array for type-safe access
 * to parsed command-line multi-value parameters (e.g., multiple -H flags).
 */
typedef enum {
    MULTI_OPTION_HEADERS,  // HTTP headers to include in request
    MULTI_OPTION_COUNT,   // Number of multi-value options (for bounds checking)
} MultiOptionsIndex;

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/**
 * MultiValueOption - Container for multi-valued command-line options
 * 
 * Stores multiple values for options that can be specified multiple times
 * (e.g., -H "header1" -H "header2").
 */
typedef struct MultiValueOption {
    int count;            // Number of values in the array
    const char **values;  // Array of string values
} MultiValueOption;

/**
 * CliArgsInfo - Parsed command-line arguments
 * 
 * Public API structure containing all parsed command-line arguments after
 * successful parsing by parse_arguments(). Uses array-based storage with
 * enum-based indexing for type-safe access to parsed values.
 * 
 * Usage:
 *   CliArgsInfo args;
 *   Error err = parse_arguments(argc, argv, &args);
 *   if (ERR_FAILED(err)) { ... }
 *   
 *   Access parsed value:
 *   const char *url = args.uri;
 *   int max_redir = args.values[VAL_MAX_REDIRECTS];
 *   bool verbose = args.flags[FLAG_VERBOSE];
 *   const char *output = args.options[OPTION_OUTPUT_FILE];
 */
typedef struct CliArgsInfo {
    Command cmd;                                      // Parsed command (GET or POST)
    Schema schema;                                    // URL schema (HTTP, HTTPS, etc.)
    const char *uri;                                  // Target URL for HTTP request
    bool flags[MAX_FLAG_COUNT];                       // Boolean flags (indexed by FlagsIndex)
    int values[MAX_VALUE_COUNT];                      // Integer values (indexed by ValuesIndex)
    const char *options[MAX_OPTION_COUNT];            // String options (indexed by OptionsIndex)
    MultiValueOption multi_options[MAX_MULTI_OPTION_COUNT];  // Multi-value options (indexed by MultiOptionsIndex)
} CliArgsInfo;

/* ============================================================================
 * Public API Functions
 * ============================================================================ */

/**
 * get_help - Display comprehensive CLI help message
 * 
 * Prints usage instructions, available commands, options, flags, and examples
 * to stdout. This function is called when the user runs 'torilate help' or
 * provides invalid arguments.
 * 
 * Output includes:
 *   - Command syntax and usage patterns
 *   - Available subcommands (GET, POST)
 *   - Command-line options (output file, max redirects, etc.)
 *   - Boolean flags (follow, verbose, raw, content-only)
 *   - Practical usage examples
 */
void get_help();

/**
 * parse_arguments - Parse and validate command-line arguments
 * 
 * Main entry point for CLI argument parsing. Validates the command, dispatches
 * to the appropriate command handler (GET or POST), and populates the provided
 * CliArgsInfo structure with parsed values.
 * 
 * This function handles:
 *   - Command validation (ensures 'get' or 'post')
 *   - Argtable3 initialization and command registration
 *   - Argument parsing and error handling
 *   - URL schema extraction and validation
 * 
 * @param argc        Argument count from main()
 * @param argv        Argument vector from main()
 * @param args_info   Pointer to CliArgsInfo structure to populate with parsed data
 * 
 * @return            Error structure (ERR_OK on success, error details on failure)
 * 
 *                    Possible error codes:
 * 
 *                      - ERR_NO_ARGS: No arguments provided  
 * 
 *                      - ERR_INVALID_COMMAND: Unknown command specified  
 * 
 *                      - ERR_INVALID_ARGS: Argument parsing failed  
 * 
 *                      - ERR_OUTOFMEMORY: Memory allocation failed  
 * 
 * Example:
 * ```
 *   CliArgsInfo args;
 *   Error err = parse_arguments(argc, argv, &args);
 *   if (ERR_FAILED(err)) {
 *       fprintf(stderr, "Error: %s\\n", err.message);
 *       return err.code;
 *   }
 * ```
 *   
 *   Use args.cmd, args.uri, args.flags[], etc.
 */
Error parse_arguments(int argc, char *argv[], CliArgsInfo *args_info);

#endif