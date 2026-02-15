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


Error net_init(void) {
    WSADATA wsa;
    int status = WSAStartup(MAKEWORD(2, 2), &wsa);
    switch (status) {
        case 0:
            return ERR_OK();
        case WSASYSNOTREADY:
            return ERR_NEW(ERR_SOCK_INIT_FAILED, "Network subsystem is not ready");
        case WSAVERNOTSUPPORTED:
            return ERR_NEW(ERR_SOCK_INIT_FAILED, "Winsock version not supported");
        case WSAEINPROGRESS:
            return ERR_NEW(ERR_SOCK_INIT_FAILED, "Blocking Winsock operation in progress");
        case WSAEPROCLIM:
            return ERR_NEW(ERR_SOCK_INIT_FAILED, "Too many processes using Winsock");
        case WSAEFAULT:
            return ERR_NEW(ERR_SOCK_INIT_FAILED, "Invalid WSADATA pointer");
        default:
            return ERR_NEW(ERR_SOCK_INIT_FAILED, "Unknown error initializing Winsock: %d", status);
    }
}

void net_cleanup(void) {
    WSACleanup();
}

void net_close(NetSocket *sock) {
    if ((SOCKET)sock->handle != INVALID_SOCKET) {
        closesocket((SOCKET)sock->handle);
        sock->handle = -1;
    }
}

Error net_connect(NetSocket *sock, const char *ip, uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        int wsa_err = WSAGetLastError();
        return ERR_NEW(ERR_SOCKET_CREATION_FAILED, "socket() creation failed with WSA error %d", wsa_err);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        closesocket(s);
        return ERR_NEW(ERR_INVALID_ADDRESS, "Failed to parse IP address: %s", ip);
    }

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int wsa_err = WSAGetLastError();
        closesocket(s);
        return ERR_NEW(ERR_CONNECTION_FAILED, "connect() failed to %s:%d with WSA error %d", ip, port, wsa_err);
    }

    sock->handle = (int)s;
    return ERR_OK();
}

Error net_send_all(NetSocket *sock, const void *buf, size_t len) {
    size_t sent = 0;
    SOCKET s = (SOCKET)sock->handle;
    const char *p = (const char*)buf;

    while (sent < len) {
        int n = send(s, p + sent, (int)(len - sent), 0);
        int wsa_err = WSAGetLastError();
        if (n < 0) {
            return ERR_NEW(ERR_NETWORK_IO, "send() failed after %zu/%zu bytes (WSA error %d)", sent, len, wsa_err);
        }
        sent += n;
    }
    return ERR_OK();
}

Error net_recv(NetSocket *sock, void *buf, size_t len, size_t *bytes_received) {
    int n = recv((SOCKET)sock->handle, (char*)buf, (int)len, 0);
    int wsa_err = WSAGetLastError();
    if (n < 0) {
        return ERR_NEW(ERR_NET_RECV_FAILED, "recv() failed with WSA error %d", wsa_err);
    }
    if (bytes_received) {
        *bytes_received = n;
    }

    return ERR_OK();
}

/* Utility functions */

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

NetAddrType net_get_addr_type(const char *addr) {
    struct in_addr ipv4_addr;
    struct in6_addr ipv6_addr;

    if (inet_pton(AF_INET, addr, &ipv4_addr) == 1)
        return IPV4;
    if (inet_pton(AF_INET6, addr, &ipv6_addr) == 1)
        return IPV6;

    return DOMAIN;
}

Error net_parse_ipv4(const char *ip, uint32_t *out) {
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) < 0) {
        int wsa_err = WSAGetLastError();
        return ERR_NEW(ERR_ADDRESS_RESOLUTION_FAILED, "Failed to parse IPv4 address '%s' with WSA error %d", ip, wsa_err);
    } else if (inet_pton(AF_INET, ip, &addr) == 0) {
        return ERR_NEW(ERR_INVALID_ADDRESS, "Invalid IPv4 address format: '%s'", ip);
    }

    *out = addr.s_addr;
    return ERR_OK();
}

#endif
