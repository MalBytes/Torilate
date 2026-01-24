/* 
    File: torialate.h 
    Author: Trident Apollo
    Date: 23-01-2026
    Socks4 RFC: https://www.openssh.org/txt/socks4.protocol
    Description: TODO
*/

#ifndef TORILATE_H
#define TORILATE_H

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>


// Return Codes
#define SUCCESS                0
#define INVALID_ARGS           1
#define WINSOCK_INIT_FAILED    2
#define SOCKET_CREATION_FAILED 3
#define INVALID_ADDRESS        4
#define TOR_FAILED             5
#define CONNECTION_FAILED      6 


// Definitions
#define PROXY_IP    "127.0.0.1"
#define PROXY_PORT  9050
#define USER_ID    "torilate"
#define REQ_SIZE    sizeof(Socks4Request)
#define RESP_SIZE   sizeof(Socks4Response)


// Custom Type Definitions
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;


// Custom Structs
typedef struct {
    uint8  vn;        // SOCKS protocol version number, 0x04 for this version
    uint8  cd;        // Command code: 0x01 = establish a TCP stream connection
    uint16 dstport;   // Destination port in network byte order
    uint32 dstip;     // Destination IP address in network byte order
    char   userid[16]; // The user ID string, variable length, null-terminated
} Socks4Request;


typedef struct {
    uint8  vn;        // SOCKS protocol version number, 0x00 for this version
    uint8  cd;        // Result code: 0x5A = request granted
    uint16 _dstport;   // Destination port in network byte order (ignored)
    uint32 _dstip;     // Destination IP address in network byte order (ignored)
} Socks4Response;

// Function Prototypes
Socks4Request *get_request(const char* dstip, const int dstport);
Socks4Response *get_response(const char* buffer);

#endif