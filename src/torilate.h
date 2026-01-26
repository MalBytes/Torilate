/* 
    File: src/torilate.h
    Author: Trident Apollo
    Date: 23-01-2026
    Reference: None
    Description:
        Public entry-point definitions and shared interfaces for Torilate.
        This header declares core data structures, constants, and function
        prototypes used by the Torilate CLI.
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

#define TOR_IP    "127.0.0.1"
#define TOR_PORT  9050
#define USER_ID   "torilate"

#ifdef _WIN32
#define PROG_NAME "torilate.exe"
#else
#define PROG_NAME "torilate"
#endif


#endif