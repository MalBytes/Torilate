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


// Return Codes
#define SUCCESS                0
#define INVALID_ARGS           1
#define SOCK_INIT_FAILED       2
#define SOCKET_CREATION_FAILED 3
#define INVALID_ADDRESS        4
#define TOR_CONNECTION_FAILED  5
#define CONNECTION_FAILED      6 
#define HTTP_REQUEST_FAILED    7


// Definitions
#define PROXY_IP    "127.0.0.1"
#define PROXY_PORT  9050
#define USER_ID    "torilate"

#endif