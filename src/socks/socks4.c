/* 
    File: src/socks/socks4.c
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - SOCKS4 Protocol:
          https://www.openssh.org/txt/socks4.protocol
    Description:
        Implementation of the SOCKS4 client-side CONNECT command.
*/

#include <string.h>
#include "socks/socks4.h"

/* Internal constants */
#define SOCKS4_VERSION      0x04
#define SOCKS4_CMD_CONNECT  0x01
#define SOCKS4_CMD_BIND     0x02

int socks4_connect(NetSocket *sock, const char *dst_ip, uint16_t dst_port, const char *user_id, Socks4AddrType addr_type) {
    uint8_t request[512];
    uint8_t response[8];
    size_t  offset = 0;
    uint32_t ip_n;

    request[offset++] = SOCKS4_VERSION;
    request[offset++] = SOCKS4_CMD_CONNECT;

    uint16_t port_n = net_htons(dst_port);
    memcpy(&request[offset], &port_n, sizeof(port_n));
    offset += sizeof(port_n);

    if (addr_type == DOMAIN) {
        if (net_parse_ipv4("0.0.0.1", &ip_n) != 0)
            return -1;
    } else {
        if (net_parse_ipv4(dst_ip, &ip_n) != 0)
            return -1;
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

    if (net_send_all(sock, request, offset) != 0)
        return -1;

    if (net_recv(sock, response, sizeof(response)) != sizeof(response))
        return -1;

    if (response[0] != 0x00 || response[1] != SOCKS4_OK)
        return -1;

    return 0;
}
