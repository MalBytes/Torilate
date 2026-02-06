/*
    File: src/error/error.h
    Author: Trident Apollo  
    Date: 6-02-2026
    Reference: None
    Description:
        Error code definitions and error handling utilities for Torilate.
*/


#ifndef TORILATE_ERROR_H
#define TORILATE_ERROR_H

#include <errno.h>
#include "torilate.h"


/* Error Codes */
#define SUCCESS                     0

// Cli Errors
#define ERR_INVALID_ARGS            1

// Network Errors
#define ERR_SOCK_INIT_FAILED        2
#define ERR_SOCKET_CREATION_FAILED  3
#define ERR_TOR_CONNECTION_FAILED   4
#define ERR_CONNECTION_FAILED       5

// HTTP Errors
#define ERR_INVALID_URI             6
#define ERR_HTTP_REQUEST_FAILED     7
#define ERR_BAD_RESPONSE            8

// System Errors
#define ERR_IO                      9
#define ERR_OUTOFMEMORY             10
#define ERR_NO_PERMISSION           11
#define ERR_FILE_NOT_FOUND          12

// Unkown Error
#define ERR_LAST_ERROR_CODE         13


/* Error Struct */
typedef struct {
    int code;
    char message[512];
} Error;

/* Function Prototypes */
const char *get_err_msg(Error *err);

#endif