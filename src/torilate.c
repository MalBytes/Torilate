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
#include "http/http.h"
#include "net/socket.h"
#include "socks/socks4.h"


int main(int argc, char *argv[]) {
    // Variable Declarations
    CliArgsInfo args;
    NetSocket sock = INVALID_SOCKET;
    int return_code = SUCCESS, status;
    int64_t bytes_received;
    
    
    // Argument validation (temporary)
    if (argc == 2 && (strcmp(argv[1], "help") == 0)) {
        get_help();
        goto cleanUp;
    } 

    status = parse_arguments(argc, argv, &args);
    if (status != SUCCESS) {
        return_code = status;
        goto cleanUp;
    }
    
    // Connect to TOR
    net_init(); // Initialize networking subsystem
    status = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (status != SUCCESS) {
        fprintf(stderr, "Failed to connect to TOR proxy at %s:%d\n", TOR_IP, TOR_PORT);
        return_code = ERR_TOR_CONNECTION_FAILED;
        goto cleanUp;
    }
    
    printf("Connected to TOR successfully!\n\n");

    // Establish SOCKS4 connection
    status = socks4_connect(&sock, args.uri.host, (uint16_t)args.uri.port, PROG_NAME, args.uri.addr_type);
    if (status != SUCCESS) {
        fprintf(stderr, "SOCKS4 connection to %s:%d failed\n", args.uri.host, args.uri.port);
        return_code = ERR_CONNECTION_FAILED;
        goto cleanUp;
    }

    printf("SOCKS4 request granted! Connected to %s:%d through TOR.\n\n", args.uri.host, args.uri.port);

    // Send HTTP request based on command
    HttpResponse resp;
    char *body_owned = NULL; // owns memory (if allocated)
    const char *body = NULL; // read-only view

    switch (args.cmd) {
        case CMD_GET:
            bytes_received = http_get(&sock, args.uri.host, args.uri.endpoint, &resp);
            if (bytes_received < 0) {
                fprintf(stderr, "%s: HTTP GET request failed\n", PROG_NAME);
                return_code = ERR_HTTP_REQUEST_FAILED;
                goto cleanUp;
            }
            printf("HTTP GET request successful! Received %lld bytes.\n", bytes_received);
            
            // Output response
            if (args.output_file) {
                int write_status = write_to(args.output_file, resp.raw, bytes_received);
                if (write_status != SUCCESS) {
                    fprintf(stderr, "%s: Failed to write response to file\n", PROG_NAME);
                    return_code = write_status;
                    goto cleanUp;
                }
                printf("%s: Response written to %s\n", PROG_NAME, args.output_file);
                break;
            } else {
                printf("Status Code: %d\n\n", resp.status_code);
                printf("Body:\n\n%s\n", resp.raw);
            }
            break;

        case CMD_POST:
            // Read body
            if (args.input_file) {
                int read_status = read_from(args.input_file, &body_owned, NULL);
                if (read_status != SUCCESS) {
                    fprintf(stderr, "%s: Failed to read file\n", PROG_NAME);
                    return_code = read_status;
                    goto cleanUp;
                }
                body = body_owned;
            } else {
                body = args.uri.body ? args.uri.body : "";
            }

            bytes_received = http_post(&sock, args.uri.host, args.uri.endpoint, args.uri.header, body, &resp);
            if (bytes_received < 0) {
                fprintf(stderr, "%s: HTTP POST request failed\n", PROG_NAME);
                return_code = ERR_HTTP_REQUEST_FAILED;
                goto cleanUp;
            }
            free(body_owned); // free if allocated
            printf("HTTP POST request successful! Received %lld bytes.\n", bytes_received);

            // Output response
            if (args.output_file) {
                int write_status = write_to(args.output_file, resp.raw, bytes_received);
                if (write_status != SUCCESS) {
                    fprintf(stderr, "%s: Failed to write response to file\n", PROG_NAME);
                    return_code = write_status;
                    goto cleanUp;
                }
                printf("%s: Response written to %s\n", PROG_NAME, args.output_file);
                break;
            } else {
                printf("Status Code: %d\n\n", resp.status_code);
                printf("Body:\n\n%s\n", resp.raw);
            }
            break;

        default:
            fprintf(stderr, "%s: Unsupported command\n", PROG_NAME);
            return_code = ERR_INVALID_ARGS;
            goto cleanUp;
    }
    
cleanUp:
    if (is_valid_socket(&sock)) {
        net_close(&sock);
    }
    net_cleanup();
    free((void*) args.uri.host);
    free((void*) args.uri.endpoint);

    return return_code;
}