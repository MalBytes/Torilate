/* 
    File: src/net/socket.h
    Author: Trident Apollo
    Date: 23-01-2026
    Reference: None
    Description:
        Platform-independent TCP socket abstraction for Torilate.
        This layer hides OS-specific networking details and exposes
        a minimal, portable API for higher-level protocols.
*/

#ifndef TORILATE_NET_SOCKET_H
#define TORILATE_NET_SOCKET_H

#include <stddef.h>
#include <stdint.h>
#include "error/error.h"

#define INVALID_SOCKET (NetSocket){ .handle = -1 } // Invalid socket representation, handle is -1

/* Opaque socket handle */
typedef struct {
    int handle;
} NetSocket;

/* Host address type */
typedef enum {
    IPV4,
    IPV6,
    DOMAIN
} NetAddrType;


/* Lifecycle */
Error net_init(void);
void net_cleanup(void);
void net_close(NetSocket *sock);

/* Connection */
Error net_connect(NetSocket *sock, const char *ip, uint16_t port);

/* I/O */
Error net_send_all(NetSocket *sock, const void *buf, size_t len);
Error net_recv(NetSocket *sock, void *buf, size_t len, size_t *bytes_received);

/* Utils */
uint16_t net_htons(uint16_t value);
uint32_t net_htonl(uint32_t value);
uint16_t net_ntohs(uint16_t value);
uint32_t net_ntohl(uint32_t value);
uint8_t is_valid_socket(NetSocket *sock);

NetAddrType net_get_addr_type(const char *addr);
Error net_parse_ipv4(const char *ip, uint32_t *out);

#endif