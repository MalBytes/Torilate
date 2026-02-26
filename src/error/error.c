/*
    File: src/error/error.c
    Author: Trident Apollo  
    Date: 6-02-2026
    Reference: None
    Description:
        Implementation of error handling functions for Torilate.
        
        Provides a robust error propagation system where errors can
        be created with context at any layer and enriched as they
        bubble up the call stack.
*/

#include "error/error.h"
#include <string.h>


/* Static error message lookup table */
static const char *err_messg_list[ERR_COUNT] = {
    [SUCCESS]                       = "No error",
    
    [ERR_NO_ARGS]                   = "No arguments provided",
    [ERR_INVALID_ARGS]              = "Invalid arguments",
    [ERR_INVALID_COMMAND]           = "Invalid command",
    
    [ERR_NETWORK_IO]                = "Network I/O error",
    [ERR_INVALID_ADDRESS]           = "Invalid network address",
    [ERR_NET_RECV_FAILED]           = "Failed to receive data from socket",
    [ERR_SOCK_INIT_FAILED]          = "Failed to initialize socket subsystem",
    [ERR_CONNECTION_FAILED]         = "Failed to connect to host",
    [ERR_TOR_CONNECTION_FAILED]     = "Failed to connect to TOR proxy",
    [ERR_SOCKET_CREATION_FAILED]    = "Failed to create socket",
    [ERR_ADDRESS_RESOLUTION_FAILED] = "Failed to resolve address",
    
    [ERR_INVALID_URI]               = "Invalid URL",
    [ERR_BAD_RESPONSE]              = "Bad or malformed response",
    [ERR_INVALID_SCHEMA]            = "Unsupported URL method or schema",
    [ERR_INVALID_HEADER]            = "Invalid HTTP header",
    [ERR_HTTP_REQUEST_FAILED]       = "HTTP request failed",
    [ERR_HTTP_REDIRECT_LIMIT]       = "Exceeded maximum HTTP redirects",
    [ERR_HTTP_REDIRECT_FAILED]      = "Failed to follow HTTP redirect",

    [ERR_IO]                        = "I/O error",
    [ERR_OUTOFMEMORY]               = "Out of memory",
    [ERR_NO_PERMISSION]             = "Permission denied",
    [ERR_FILE_NOT_FOUND]            = "File not found",

    [ERR_LAST_ERROR_CODE]           = "Unknown error"
};

static int nerr = sizeof(err_messg_list) / sizeof(err_messg_list[0]) - 1;


const char *err_get_base_message(ErrorCode code) {
    if ((int)code < 0 || (int)code >= nerr) {
        return err_messg_list[ERR_LAST_ERROR_CODE];
    }
    return err_messg_list[code];
}

/*
 * Helper: Extract top level error from propagation chain.
 * The top level error is everything before the first ": " separator.
 * If no separator found, returns the full message.
 */
static const char *extract_top_level_error(const char *message) {
    static char buffer[512];
    const char *first_separator = strstr(message, ": ");
    
    // If we found a separator, copy everything before it
    if (first_separator) {
        size_t len = first_separator - message;
        if (len >= sizeof(buffer)) len = sizeof(buffer) - 1;
        memcpy(buffer, message, len);
        buffer[len] = '\0';
        return buffer;
    }
    
    // Otherwise, return the full message
    return message;
}

const char *get_err_msg(Error *err, bool verbose) {
    int err_code = ((int)err->code < 0 || (int)err->code >= nerr) ? ERR_LAST_ERROR_CODE : (int)err->code;
    char buffer[512];
    const char *message_to_display = err->message;

    // In simple mode, extract only the top-level error
    if (!verbose && err->message[0] != '\0') {
        message_to_display = extract_top_level_error(err->message);
    }

    // Format the final error message
    if (message_to_display[0] != '\0') {
        snprintf(buffer, sizeof(buffer), "%s: (%d) %s: %s", 
                PROG_NAME, err->code, err_messg_list[err_code], message_to_display);
    } else {
        snprintf(buffer, sizeof(buffer), "%s: (%d) %s", 
                PROG_NAME, err->code, err_messg_list[err_code]);
    }
    
    snprintf(err->message, sizeof(err->message), "%s", buffer);
    return (const char *)err->message;
}

Error err_create(ErrorCode code, const char *fmt, ...) {
    Error err;
    err.code = code;
    
    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(err.message, sizeof(err.message), fmt, args);
        va_end(args);
    } else {
        err.message[0] = '\0';
    }
    
    return err;
}

Error err_propagate(Error err, const char *fmt, ...) {
    if (!fmt) {
        return err; // No additional context, return as-is
    }
    
    char context[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(context, sizeof(context), fmt, args);
    va_end(args);
    
    // If original error has a message, append context
    if (err.message[0] != '\0') {
        char combined[512];
        snprintf(combined, sizeof(combined), "%s: %s", context, err.message);
        snprintf(err.message, sizeof(err.message), "%s", combined);
    } else {
        // Otherwise, just set the context as the message
        snprintf(err.message, sizeof(err.message), "%s", context);
    }
    
    return err;
}
