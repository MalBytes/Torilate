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
    
    
    // Argument validation (temporary)
    if (argc == 2 && (strcmp(argv[1], "help") == 0)) {
        get_help();
        goto cleanUp;
    } else if (return_code = parse_arguments(argc, argv, &args) != 0) {
        goto cleanUp;
    }
    
    // Connect to TOR
    net_init(); // Initialize networking subsystem
    status = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (status != 0) {
        fprintf(stderr, "Failed to connect to TOR proxy at %s:%d\n", TOR_IP, TOR_PORT);
        return_code = TOR_CONNECTION_FAILED;
        goto cleanUp;
    }
    
    printf("Connected to TOR successfully!\n\n");

    // Establish SOCKS4 connection
    status = socks4_connect(&sock, args.host, (uint16_t)args.port, USER_ID, args.addr_type);
    if (status != 0) {
        fprintf(stderr, "SOCKS4 connection to %s:%d failed\n", args.host, args.port);
        return_code = CONNECTION_FAILED;
        goto cleanUp;
    }

    printf("SOCKS4 request granted! Connected to %s:%d through TOR.\n\n", args.host, args.port);

    // Test HTTP GET request
    HttpResponse resp;
    status = http_get(&sock, args.host, args.endpoint, &resp);
    if (status < 0) {
        fprintf(stderr, "HTTP GET request failed\n");
        return_code = HTTP_REQUEST_FAILED;
        goto cleanUp;
    }

    printf("HTTP GET request successful! Received %d bytes.\n", status);
    printf("Response Status Code: %d\n\n", resp.status_code);
    printf("Response Body:\n\n%s\n", resp.raw);

  
cleanUp:
    if (is_valid_socket(&sock)) {
        net_close(&sock);
    }
    net_cleanup();

    return return_code;
}