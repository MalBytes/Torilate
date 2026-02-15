/* 
    File: src/net/socket_posix.c
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - POSIX socket API: https://man7.org/linux/man-pages/man2/socket.2.html
    Description:
        POSIX-compliant implementation of the Torilate socket
        abstraction for Linux and Unix-like systems.
*/

#ifndef _WIN32

#include <errno.h>
#include <unistd.h>
#include "net/socket.h"
#include <arpa/inet.h>
#include <sys/socket.h>

Error net_init(void) {
    return ERR_OK(); /* no-op */
}

void net_cleanup(void) {
    /* no-op */
}

void net_close(NetSocket *sock) {
    if (sock->handle >= 0) {
        close(sock->handle);
        sock->handle = -1;
    }
}

Error net_connect(NetSocket *sock, const char *ip, uint16_t port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        return ERR_NEW(ERR_SOCKET_CREATION_FAILED, "socket() creation failed with error %d", errno);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        close(s);
        return ERR_NEW(ERR_INVALID_ADDRESS, "Failed to parse IP address '%s'", ip);
    }

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        int err = errno;
        close(s);
        return ERR_NEW(ERR_CONNECTION_FAILED, "Failed to connect to %s:%d with error %d", ip, port, err);
    }

    sock->handle = s;
    return ERR_OK();
}

Error net_send_all(NetSocket *sock, const void *buf, size_t len) {
    size_t sent = 0;
    const char *p = (const char*)buf;

    while (sent < len) {
        ssize_t n = send(sock->handle, p + sent, len - sent, 0);
        int err = errno;
        if (n < 0)
            return ERR_NEW(ERR_NETWORK_IO, "send() failed after %zu/%zu bytes (error %d)", sent, len, err);
        sent += n;
    }
    return ERR_OK();
}

Error net_recv(NetSocket *sock, void *buf, size_t len, size_t *bytes_received) {
    ssize_t n = recv(sock->handle, buf, len, 0);
    int err = errno;
    if (n < 0) {
        return ERR_NEW(ERR_NETWORK_IO, "recv() failed with error %d", err);
    }
    if (bytes_received) {
        *bytes_received = n;
    }

    return ERR_OK();
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
    if (inet_pton(AF_INET, ip, &addr) < 1) {
        return ERR_NEW(ERR_ADDRESS_RESOLUTION_FAILED, "Failed to parse IPv4 address '%s' with error %d", ip, errno);
    } else if (inet_pton(AF_INET, ip, &addr) == 0) {
        return ERR_NEW(ERR_INVALID_ADDRESS, "Invalid IPv4 address format: '%s'", ip);
    }

    *out = addr.s_addr;
    return ERR_OK();
}

#endif
