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

int net_init(void) {
    return 0; /* no-op */
}

void net_cleanup(void) {
    /* no-op */
}

int net_connect(NetSocket *sock, const char *ip, uint16_t port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        close(s);
        return -1;
    }

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    sock->handle = s;
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

NetAddrType net_get_addr_type(const char *addr) {
    struct in_addr ipv4_addr;
    struct in6_addr ipv6_addr;

    if (inet_pton(AF_INET, addr, &ipv4_addr) == 1)
        return IPV4;
    if (inet_pton(AF_INET6, addr, &ipv6_addr) == 1)
        return IPV6;

    return DOMAIN;
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
    const char *p = (const char*)buf;

    while (sent < len) {
        ssize_t n = send(sock->handle, p + sent, len - sent, 0);
        if (n <= 0)
            return -1;
        sent += n;
    }
    return 0;
}

int net_recv(NetSocket *sock, void *buf, size_t len) {
    return recv(sock->handle, buf, len, 0);
}

void net_close(NetSocket *sock) {
    if (sock->handle >= 0) {
        close(sock->handle);
        sock->handle = -1;
    }
}

int net_last_error(void) {
    return errno;
}

#endif
