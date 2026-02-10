/*
    File: src/error/error.c
    Author: Trident Apollo  
    Date: 6-02-2026
    Reference: None
    Description:
        Implementation of error handling functions for Torilate.
*/

#include "error/error.h"


static const char *err_messg_list[ERR_COUNT] = {
    [SUCCESS]                    = "No error",

    [ERR_NO_ARGS]                = "No arguments provided",
    [ERR_INVALID_ARGS]           = "Invalid arguments",
    [ERR_INVALID_COMMAND]        = "Invalid command",

    [ERR_NETWORK_IO]             = "Network I/O error",
    [ERR_INVALID_ADDRESS]        = "Invalid network address",
    [ERR_SOCK_INIT_FAILED]       = "Failed to initialize socket subsystem",
    [ERR_CONNECTION_FAILED]      = "Failed to connect to host",
    [ERR_TOR_CONNECTION_FAILED]  = "Failed to connect to TOR proxy",
    [ERR_SOCKET_CREATION_FAILED] = "Failed to create socket",
    
    [ERR_INVALID_URI]            = "Invalid URL",
    [ERR_BAD_RESPONSE]           = "Bad or malformed response",
    [ERR_HTTP_REQUEST_FAILED]    = "HTTP request failed",

    [ERR_IO]                     = "I/O error",
    [ERR_OUTOFMEMORY]            = "Out of memory",
    [ERR_NO_PERMISSION]          = "Permission denied",
    [ERR_FILE_NOT_FOUND]         = "File not found",

    [ERR_LAST_ERROR_CODE]        = "Unknown error"
};

static int nerr = sizeof(err_messg_list) / sizeof(err_messg_list[0]) - 1;

const char *get_err_msg(Error *err) {
    int err_code = (err->code < 0 || err->code >= nerr) ? ERR_LAST_ERROR_CODE : err->code;
    char buffer[512];

    if (err->message[0] != '\0') {
        snprintf(buffer, sizeof(buffer), "%s: (%d) %s: %s", PROG_NAME, err->code, err_messg_list[err_code], err->message);
    } else {
        snprintf(buffer, sizeof(buffer), "%s: (%d) %s", PROG_NAME, err->code, err_messg_list[err_code]);
    }
    snprintf(err->message, sizeof(err->message), "%s", buffer);

    return (const char *)err->message;
}
