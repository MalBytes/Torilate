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
#define SUCCESS                     0
#define ERR_INVALID_ARGS            1
#define ERR_SOCK_INIT_FAILED        2
#define ERR_SOCKET_CREATION_FAILED  3
#define ERR_INVALID_URI             4
#define ERR_TOR_CONNECTION_FAILED   5
#define ERR_CONNECTION_FAILED       6 
#define ERR_HTTP_REQUEST_FAILED     7
#define ERR_OUTOFMEMORY             8
#define ERR_NO_PERMISSION           9
#define ERR_FILE_NOT_FOUND          10
#define ERR_IO                      11


// Definitions
#define TOR_IP          "127.0.0.1"
#define TOR_PORT        9050
#define PROG_NAME       "torilate"


#define VER_MAJOR   0
#define VER_MINOR   1
#define VER_PATCH   2
#define VER_TAG     "alpha"


typedef enum {
    HTTP,
    HTTPS,
    INVALID_SCHEMA,
} Schema;

#endif