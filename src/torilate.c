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
    int status = 0;
    Error error = {0};
    CliArgsInfo args = {0};
    NetSocket sock = INVALID_SOCKET;

    // Initialize error structure
    error.code = SUCCESS;
    memset(error.message, 0, sizeof(error.message));
    
    // Argument validation (temporary)
    if (argc == 2 && (strcmp(argv[1], "help") == 0)) {
        get_help();
        goto cleanUp;
    } 

    status = parse_arguments(argc, argv, &args);
    if (status != SUCCESS) {
        error.code = status;
        goto cleanUp;
    }
    bool raw = args.flags[FLAG_RAW] == true;
    
    // Connect to TOR
    net_init(); // Initialize networking subsystem
    status = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (status != SUCCESS) {
        snprintf(error.message, sizeof(error.message), "Cannot connect to TOR at %s:%d", TOR_IP, TOR_PORT);
        error.code = ERR_TOR_CONNECTION_FAILED;
        goto cleanUp;
    }

    // Establish SOCKS4 connection
    status = socks4_connect(&sock, args.uri.host, (uint16_t)args.uri.port, PROG_NAME, args.uri.addr_type);
    if (status != SUCCESS) {
        snprintf(error.message, sizeof(error.message), "SOCKS4 connection to %s:%d failed", args.uri.host, args.uri.port);
        error.code = ERR_CONNECTION_FAILED;
        goto cleanUp;
    }

    // Send HTTP request based on command
    HttpResponse resp;
    size_t resp_size = 0;
    char *body_owned = NULL; // owns memory (if allocated)
    const char *body = NULL; // read-only view
    char parsed_response[HTTP_MAX_RESPONSE] = {0};

    switch (args.cmd) {
        case CMD_GET:
            status = http_get(&sock, args.uri.host, args.uri.endpoint, &resp);
            if (status != SUCCESS) {
                snprintf(error.message, sizeof(error.message), "HTTP GET request to %s:%d failed", args.uri.host, args.uri.port);
                error.code = ERR_HTTP_REQUEST_FAILED;
                goto cleanUp;
            }
            // printf("Received %llu bytes\n\n", resp.bytes_received); // TODO: for verbose mode
            
            // Parse response
            status = parse_http_response(&resp, parsed_response, sizeof(parsed_response), &resp_size, raw);
            if (status != SUCCESS) {
                snprintf(error.message, sizeof(error.message), "Failed to parse HTTP response");
                error.code = status;
                goto cleanUp;
            }
            
            // Output response
            if (args.options[OPTION_OUTPUT_FILE]) {
                int write_status = write_to(args.options[OPTION_OUTPUT_FILE], parsed_response, resp_size);
                if (write_status != SUCCESS) {
                    snprintf(error.message, sizeof(error.message), "Failed to write response to file %s", args.options[OPTION_OUTPUT_FILE]);
                    error.code = write_status;
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
                int read_status = read_from(args.options[OPTION_INPUT_FILE], &body_owned, NULL);
                if (read_status != SUCCESS) {
                    snprintf(error.message, sizeof(error.message), "Failed to read file %s", args.options[OPTION_INPUT_FILE]);
                    error.code = read_status;
                    goto cleanUp;
                }
                body = body_owned;
            } else {
                body = args.uri.body ? args.uri.body : "";
            }

            status = http_post(&sock, args.uri.host, args.uri.endpoint, args.uri.header, body, &resp);
            if (status != SUCCESS) {
                snprintf(error.message, sizeof(error.message), "HTTP POST request to %s:%d failed", args.uri.host, args.uri.port);
                error.code = ERR_HTTP_REQUEST_FAILED;
                goto cleanUp;
            }
            free(body_owned); // free if allocated
            // printf("Received %llu bytes\n\n", resp.bytes_received); // TODO: for verbose mode

            // Parse response
            status = parse_http_response(&resp, parsed_response, sizeof(parsed_response), &resp_size, raw);
            if (status != SUCCESS) {
                snprintf(error.message, sizeof(error.message), "Failed to parse HTTP response");
                error.code = status;
                goto cleanUp;
            }

            // Output response
            if (args.options[OPTION_OUTPUT_FILE]) {
                int write_status = write_to(args.options[OPTION_OUTPUT_FILE], parsed_response, resp_size);
                if (write_status != SUCCESS) {
                    snprintf(error.message, sizeof(error.message), "Failed to write response to file %s", args.options[OPTION_OUTPUT_FILE]);
                    error.code = write_status;
                    goto cleanUp;
                }
                printf("%s: Response written to %s\n", PROG_NAME, args.options[OPTION_OUTPUT_FILE]);
                break;
            } else {
                fwrite(parsed_response, 1, resp_size, stdout);
            }
            break;

        default:
            snprintf(error.message, sizeof(error.message), "Unsupported command");
            error.code = ERR_INVALID_ARGS;
            goto cleanUp;
    }
    
cleanUp:
    if (is_valid_socket(&sock)) {
        net_close(&sock);
    }
    net_cleanup();
    free((void*) args.uri.host);
    free((void*) args.uri.endpoint);

    // Print error message
    if (error.code != SUCCESS) {
        printf("%s\n", get_err_msg(&error));
    }

    return error.code;
}