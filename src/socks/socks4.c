/* 
    File: src/socks/socks4.c
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - SOCKS4 Protocol: https://www.openssh.org/txt/socks4.protocol
        - SOCKS4a Extension: https://www.openssh.org/txt/socks4a.protocol
    Description:
        Implementation of the SOCKS4 client-side CONNECT command.
*/

#include <string.h>
#include "socks/socks4.h"

/* Internal constants */
#define SOCKS4_VERSION      0x04
#define SOCKS4_CMD_CONNECT  0x01
#define SOCKS4_CMD_BIND     0x02

Error socks4_connect(NetSocket *sock, const char *dst_ip, uint16_t dst_port, const char *user_id, NetAddrType addr_type) {
    uint32_t ip_n;
    size_t  offset = 0;
    uint8_t response[8];
    uint8_t request[512];
    Error err = ERR_OK();

    request[offset++] = SOCKS4_VERSION;
    request[offset++] = SOCKS4_CMD_CONNECT;

    uint16_t port_n = net_htons(dst_port);
    memcpy(&request[offset], &port_n, sizeof(port_n));
    offset += sizeof(port_n);

    if (addr_type == DOMAIN) {
        err = net_parse_ipv4("0.0.0.1", &ip_n);
        if (ERR_FAILED(err))
            return ERR_PROPAGATE(err, "Failed to set SOCKS4 domain placeholder IP");
    } else {
        err = net_parse_ipv4(dst_ip, &ip_n);
        if (ERR_FAILED(err))
            return ERR_PROPAGATE(err, "SOCKS4 IP resolution failed");
    }

    memcpy(&request[offset], &ip_n, sizeof(ip_n));
    offset += sizeof(ip_n);

    if (user_id && user_id[0]) {
        size_t len = strlen(user_id);
        memcpy(&request[offset], user_id, len);
        offset += len;
    }
    request[offset++] = '\0';

    if (addr_type == DOMAIN) {
        size_t host_len = strlen(dst_ip);
        memcpy(&request[offset], dst_ip, host_len);
        offset += host_len;
    }
    request[offset++] = '\0';
    
    err = net_send_all(sock, request, offset);
    if (ERR_FAILED(err))
        // Preserves: bytes sent, WSA error, etc.
        return ERR_PROPAGATE(err, "Failed to send SOCKS4 CONNECT request (%zu bytes)", offset);

    size_t bytes_received;
    err = net_recv(sock, response, sizeof(response), &bytes_received);
    if (ERR_FAILED(err)) {
        return ERR_PROPAGATE(err, "Failed to receive SOCKS4 response");
    }
    
    if (bytes_received != sizeof(response)) {
        return ERR_NEW(ERR_NET_RECV_FAILED, "Expected 8 bytes in SOCKS4 response but received %zu", bytes_received);
    }

    if (response[0] != 0x00 || response[1] != SOCKS4_OK) {
        return ERR_NEW(ERR_CONNECTION_FAILED, "SOCKS4 request rejected (VN=%d, CD=%d) for %s:%d", response[0], response[1], dst_ip, dst_port);
    }

    return err;
}
