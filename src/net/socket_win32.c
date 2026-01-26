/* 
    File: src/net/socket_win32.c
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - Winsock 2 API: https://learn.microsoft.com/en-us/windows/win32/winsock/
    Description:
        Windows-specific implementation of the Torilate socket
        abstraction using Winsock2.
*/

#ifdef _WIN32

#include "net/socket.h"
#include <winsock2.h>
#include <ws2tcpip.h>


int net_init(void) {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa);
}

void net_cleanup(void) {
    WSACleanup();
}

int net_connect(NetSocket *sock, const char *ip, uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        closesocket(s);
        return -1;
    }

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return -1;
    }

    sock->handle = (int)s;
    return 0;
}

uint16_t net_htons(uint16_t value) {
    return htons(value);
}

uint32_t net_htonl(uint32_t value) {
    return htonl(value);
}

uint16_t net_ntohs(uint16_t value) {
    return ntohs(value);
}

uint32_t net_ntohl(uint32_t value) {
    return ntohl(value);
}

uint8_t is_valid_socket(NetSocket *sock) {
    return sock->handle != -1;
}

int net_parse_ipv4(const char *ip, uint32_t *out) {
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) != 1)
        return -1;

    *out = addr.s_addr;
    return 0;
}

int net_send_all(NetSocket *sock, const void *buf, size_t len) {
    size_t sent = 0;
    SOCKET s = (SOCKET)sock->handle;
    const char *p = (const char*)buf;

    while (sent < len) {
        int n = send(s, p + sent, (int)(len - sent), 0);
        if (n <= 0)
            return -1;
        sent += n;
    }
    return 0;
}

int net_recv(NetSocket *sock, void *buf, size_t len) {
    return recv((SOCKET)sock->handle, (char*)buf, (int)len, 0);
}

void net_close(NetSocket *sock) {
    if ((SOCKET)sock->handle != INVALID_SOCKET) {
        closesocket((SOCKET)sock->handle);
        sock->handle = -1;
    }
}

int net_last_error(void) {
    return WSAGetLastError();
}

#endif
