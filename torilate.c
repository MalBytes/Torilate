/* 
    File: torialate.c 
    Author: Trident Apollo
    Date: 23-01-2026
    Socks4 RFC: https://www.openssh.org/txt/socks4.protocol
    Description: TODO
*/

#include "torilate.h"


int main(int argc, char *argv[]) {
    // Variable Declarations
    int port;
    char *host;
    WSADATA wsa;
    SOCKADDR_IN proxy;
    SOCKET sock = INVALID_SOCKET;
    Socks4Request *req = NULL;
    Socks4Response *resp = NULL;
    char response_buffer[RESP_SIZE];
    int return_code = SUCCESS;
    
    // Argument validation
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        return INVALID_ARGS;
    }

    host = argv[1];
    port = atoi(argv[2]);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed (WSA error: %d)\n", WSAGetLastError());
        return_code = WINSOCK_INIT_FAILED;
        goto cleanUp;
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed (WSA error: %d)\n", WSAGetLastError());
        return_code = SOCKET_CREATION_FAILED;
        goto cleanUp;
    }

    // Setup address
    proxy.sin_family = AF_INET;
    proxy.sin_port   = htons(PROXY_PORT);
    if (inet_pton(AF_INET, PROXY_IP, &proxy.sin_addr) != 1) {
        fprintf(stderr, "Invalid Proxy IP address (WSA error: %d)\n", WSAGetLastError());
        return_code = INVALID_ADDRESS;
        goto cleanUp;
    }

    // Connect to proxy
    if (connect(sock, (struct sockaddr*)&proxy, sizeof(proxy)) == SOCKET_ERROR) {
        fprintf(stderr, "Failed to connect to TOR network! (WSA error: %d)\n", WSAGetLastError());
        printf("Please ensure TOR service is running on %s:%d?\n", PROXY_IP, PROXY_PORT);
        return_code = TOR_FAILED;
        goto cleanUp;
    }

    printf("Connected to TOR successfully!\n");

    // Send SOCKS4 connect request
    req = get_request(host, port);
    send(sock, (const char *)&req->vn, 1, 0);
    send(sock, (const char *)&req->cd, 1, 0); 
    send(sock, (const char *)&req->dstport, 2, 0); 
    send(sock, (const char *)&req->dstip, 4, 0); 
    send(sock, req->userid, strlen(req->userid) + 1, 0);

    // Receive SOCKS4 response
    memset(response_buffer, 0, RESP_SIZE);
    int nrecv = recv(sock, response_buffer, RESP_SIZE, 0);
    if (nrecv == 0) {
        fprintf(stderr, "Connection closed by proxy\n");
        return_code = CONNECTION_FAILED;
        goto cleanUp;
    } else if (nrecv < SOCKET_ERROR) {
        fprintf(stderr, "Response Receive failed (WSA error: %d)\n", WSAGetLastError());
        return_code = CONNECTION_FAILED;
        goto cleanUp;
    }
    resp = get_response(response_buffer);
    /* SOCKS4 Response Codes (cd):
        90: request granted
        91: request rejected or failed
        92: request rejected becasue SOCKS server cannot connect to
            identd on the client
        93: request rejected because the client program and identd
            report different user-ids
    */
    if (resp->cd != 90) { 
        fprintf(stderr, "SOCKS4 request rejected or failed (code: %d)\n", resp->cd);
        return_code = CONNECTION_FAILED;
        goto cleanUp;
    }

    printf("SOCKS4 request granted! You are now connected to %s:%d through TOR.\n", host, port);

    // Test data transfer
    char data[512] = "GET / HTTP/1.1\r\nHost: ";
    strcat(data, host);
    strcat(data, "\r\nConnection: close\r\n\r\n");
    send(sock, data, strlen(data), 0);
    memset(data, 0, sizeof(data));
    nrecv = recv(sock, data, sizeof(data) - 1, 0);
    if (nrecv > 0) {
        printf("Received %d bytes from %s:\n'%s'\n", nrecv, host, data);
    } else {
        fprintf(stderr, "Data Receive failed (WSA error: %d)\n", WSAGetLastError());
        return_code = CONNECTION_FAILED;
        goto cleanUp;
    }

  
cleanUp:
    if (sock != INVALID_SOCKET)
        closesocket(sock);
    if (req != NULL)
        free(req);
    if (resp != NULL)
        free(resp);
    
    WSACleanup();

    return return_code;
}


Socks4Request *get_request(const char* dstip, const int dstport) {
    Socks4Request *req = (Socks4Request*)malloc(REQ_SIZE);
    if (!req) {
        return NULL; // Memory allocation failed
    }

    req->vn = 0x04; // SOCKS version 4
    req->cd = 0x01; // Command code: establish a TCP stream connection
    req->dstport = htons(dstport); // Destination port in network byte order

    // Convert destination IP from string to uint32 in network byte order
    struct in_addr addr;
    if (inet_pton(AF_INET, dstip, &addr) != 1) {
        free(req);
        return NULL; // Invalid IP address
    }
    req->dstip = addr.s_addr;

    // Set userid
    snprintf(req->userid, sizeof(req->userid), "%s", USER_ID);

    return req;
}

Socks4Response *get_response(const char* buffer) {
    Socks4Response *resp = (Socks4Response*)malloc(RESP_SIZE);
    if (!resp) {
        return NULL; // Memory allocation failed
    }

    memcpy(resp, buffer, RESP_SIZE);
    return resp;
}