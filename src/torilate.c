/* 
    File: torialate.c 
    Author: Trident Apollo
    Date: 23-01-2026
    Socks4 RFC: https://www.openssh.org/txt/socks4.protocol
    Description: TODO
*/

#include "torilate.h"
#include "http/http.h"
#include "net/socket.h"
#include "socks/socks4.h"


int main(int argc, char *argv[]) {
    // Variable Declarations
    int port;
    char *host;
    char *endpoint;
    NetSocket sock;
    int return_code = SUCCESS, status;
    Socks4AddrType addr_type;
    
    // Argument validation
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        return INVALID_ARGS;
    }

    if (strcmp(argv[1], "-ns") == 0) {
        host = argv[2];
        port = 80;
        addr_type = DOMAIN;
        endpoint = argc == 4 ? argv[3] : "/";
    } else {
        host = argv[1];
        port = atoi(argv[2]);
        addr_type = IPV4;
    }
    

    net_init();
    
    // Connect to TOR
    status   = net_connect(&sock, PROXY_IP, PROXY_PORT); 
    if (status != 0) {
        fprintf(stderr, "Failed to connect to TOR proxy at %s:%d\n", PROXY_IP, PROXY_PORT);
        return_code = TOR_CONNECTION_FAILED;
        goto cleanUp;
    }
    
    printf("Connected to TOR successfully!\n\n");

    // Establish SOCKS4 connection
    status = socks4_connect(&sock, host, (uint16_t)port, USER_ID, addr_type);
    if (status != 0) {
        fprintf(stderr, "SOCKS4 connection to %s:%d failed\n", host, port);
        return_code = CONNECTION_FAILED;
        goto cleanUp;
    }

    printf("SOCKS4 request granted! Connected to %s:%d through TOR.\n\n", host, port);

    // Test HTTP GET request
    HttpResponse resp;
    status = http_get(&sock, host, endpoint, &resp);
    if (status < 0) {
        fprintf(stderr, "HTTP GET request failed\n");
        return_code = HTTP_REQUEST_FAILED;
        goto cleanUp;
    }

    printf("HTTP GET request successful! Received %d bytes.\n", status);
    printf("Response Status Code: %d\n\n", resp.status_code);
    printf("Response Body:\n\n%s\n", resp.raw);

  
cleanUp:
    if (sock.handle != -1) {
        net_close(&sock);
    }
    net_cleanup();

    return return_code;
}