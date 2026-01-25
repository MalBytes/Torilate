/* 
    File: src/socks/socks4.h
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - SOCKS4 Protocol:
          https://www.openssh.org/txt/socks4.protocol
    Description:
        SOCKS4 client-side protocol implementation.
        Provides functionality to establish TCP connections
        through a SOCKS4 proxy over an existing network socket.
*/

#ifndef TORILATE_SOCKS4_H
#define TORILATE_SOCKS4_H

#include <stdint.h>
#include "net/socket.h"

/* SOCKS4 result codes */
typedef enum {
    SOCKS4_OK                = 90,
    SOCKS4_REJECTED          = 91,
    SOCKS4_IDENTD_UNREACH    = 92,
    SOCKS4_IDENTD_MISMATCH   = 93
} Socks4Status;

/*
 * Establish a SOCKS4 CONNECT tunnel.
 *
 * Parameters:
 *   sock     - connected socket to SOCKS4 proxy
 *   dst_ip   - destination IPv4 address (dotted-decimal)
 *   dst_port - destination port (host byte order)
 *   user_id  - user ID string (may be NULL or empty)
 *
 * Returns:
 *   0 on success
 *  -1 on protocol or transport error
 */
int socks4_connect(NetSocket *sock,
                   const char *dst_ip,
                   uint16_t dst_port,
                   const char *user_id);

#endif /* TORILATE_SOCKS4_H */
