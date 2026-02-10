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
typedef enum {
    SUCCESS = 0,

    /* CLI errors */
    ERR_INVALID_ARGS,

    /* Network errors */
    ERR_NETWORK_IO,
    ERR_INVALID_ADDRESS,
    ERR_SOCK_INIT_FAILED,
    ERR_CONNECTION_FAILED,
    ERR_SOCKET_CREATION_FAILED,
    ERR_TOR_CONNECTION_FAILED,

    /* HTTP errors */
    ERR_INVALID_URI,
    ERR_BAD_RESPONSE,
    ERR_HTTP_REQUEST_FAILED,

    /* System errors */
    ERR_IO,
    ERR_OUTOFMEMORY,
    ERR_NO_PERMISSION,
    ERR_FILE_NOT_FOUND,

    /* Unknown errors */
    ERR_LAST_ERROR_CODE,

    /* Sentinel */
    ERR_COUNT
} ErrorCode;


/* Error Struct */
typedef struct {
    ErrorCode code;
    char message[512];
} Error;

/* Function Prototypes */
const char *get_err_msg(Error *err);

#endif