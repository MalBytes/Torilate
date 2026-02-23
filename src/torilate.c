/* 
    File: src/torilate.c
    Author: Trident Apollo
    Date: 23-01-2026
    Reference: None
    Description:
        Main entry point for the Torilate application.
        Responsible for parsing command-line arguments, initializing
        required subsystems, establishing Tor-backed network tunnels,
        and dispatching requests to protocol-specific handlers such as
        HTTP over SOCKS.
*/

#include "torilate.h"
#include "cli/cli.h"
#include "util/util.h"
#include "http/http.h"
#include "net/socket.h"
#include "error/error.h"
#include "socks/socks4.h"

#include <stdbool.h>


int main(int argc, char *argv[]) {
    // Variable Declarations (initialized to default values or NULL)
    Error error = {0};
    CliArgsInfo args = {0};

    // Initialize error structure
    error.code = SUCCESS;
    memset(error.message, 0, sizeof(error.message));
    
    // Argument validation (temporary)
    if (argc == 2 && (strcmp(argv[1], "help") == 0)) {
        get_help();
        goto cleanUp;
    } 

    error = parse_arguments(argc, argv, &args);
    if (ERR_FAILED(error)) {
        goto cleanUp;
    }

    // Extract flags and values for easier access
    bool raw = args.flags[FLAG_RAW] == true;
    bool follow = args.flags[FLAG_FOLLOW] == true;
    bool content_only = args.flags[FLAG_CONTENT_ONLY] == true;

    int max_redirects = args.values[VAL_MAX_REDIRECTS];

    net_init(); // Initialize networking subsystem

    // Send HTTP request based on command
    HttpResponse resp;
    size_t resp_size = 0;
    char *body_owned = NULL; // owns memory (if allocated)
    const char *body = NULL; // read-only view
    char parsed_response[HTTP_MAX_RESPONSE] = {0};

    switch (args.cmd) {
        case CMD_GET:
            error = http_get(args.uri, follow, max_redirects, &resp);
            if (ERR_FAILED(error)) {
                error = ERR_PROPAGATE(error, "HTTP GET request to URL '%s' failed", args.uri);
                goto cleanUp;
            }
            
            // Parse response
            error = parse_http_response(&resp, parsed_response, sizeof(parsed_response), &resp_size, raw, content_only);
            if (ERR_FAILED(error)) {
                error = ERR_PROPAGATE(error, "Failed to parse HTTP response");
                goto cleanUp;
            }
            
            // Output response
            if (args.options[OPTION_OUTPUT_FILE]) {
                error = write_to(args.options[OPTION_OUTPUT_FILE], parsed_response, resp_size);
                if (ERR_FAILED(error)) {
                    error = ERR_PROPAGATE(error, "Failed to write response to file %s", args.options[OPTION_OUTPUT_FILE]);
                    goto cleanUp;
                }
                printf("%s: Response written to %s\n", PROG_NAME, args.options[OPTION_OUTPUT_FILE]);
                break;
            } else {
                fwrite(parsed_response, 1, resp_size, stdout);
            }
            break;

        case CMD_POST:
            // Read body
            if (args.options[OPTION_INPUT_FILE]) {
                error = read_from(args.options[OPTION_INPUT_FILE], &body_owned, NULL);
                if (ERR_FAILED(error)) {
                    error = ERR_PROPAGATE(error, "Failed to read file %s", args.options[OPTION_INPUT_FILE]);
                    goto cleanUp;
                }
                body = body_owned;
            } else {
                body = args.options[OPTION_BODY];
            }

            error = http_post(args.uri, args.options[OPTION_HEADER], body, follow, max_redirects, &resp);
            if (ERR_FAILED(error)) {
                error = ERR_PROPAGATE(error, "HTTP POST request to URL '%s' failed", args.uri);
                error.code = ERR_HTTP_REQUEST_FAILED;
                goto cleanUp;
            }
            free(body_owned); // free if allocated

            // Parse response
            error = parse_http_response(&resp, parsed_response, sizeof(parsed_response), &resp_size, raw, content_only);
            if (ERR_FAILED(error)) {
                error = ERR_PROPAGATE(error, "Failed to parse HTTP response");
                goto cleanUp;
            }

            // Output response
            if (args.options[OPTION_OUTPUT_FILE]) {
                error = write_to(args.options[OPTION_OUTPUT_FILE], parsed_response, resp_size);
                if (ERR_FAILED(error)) {
                    error = ERR_PROPAGATE(error, "Failed to write response to file %s", args.options[OPTION_OUTPUT_FILE]);
                    goto cleanUp;
                }
                printf("%s: Response written to %s\n", PROG_NAME, args.options[OPTION_OUTPUT_FILE]);
                break;
            } else {
                fwrite(parsed_response, 1, resp_size, stdout);
            }
            break;

        default:
            error = ERR_NEW(ERR_INVALID_COMMAND, "Unsupported command");
            goto cleanUp;
    }

    if (args.flags[FLAG_VERBOSE]) {
        printf("\n");
        printf("%s: Request to URL '%s' completed successfully\n", PROG_NAME, args.uri);
        printf("%s: Status Code: %d, Bytes Received: %llu\n", PROG_NAME, resp.status_code, resp.bytes_received);
    }
    
cleanUp:
    net_cleanup();

    // Print error message
    bool verbose = args.flags[FLAG_VERBOSE];
    if (error.code >= ERR_NO_ARGS && error.code <= ERR_INVALID_COMMAND) {
        // CLI errors: always print full message (verbose or not)
        verbose = true;
    }
    if (error.code != SUCCESS) {
        printf("%s\n", get_err_msg(&error, verbose));
    }

    return error.code;
}