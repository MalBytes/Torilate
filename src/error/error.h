/*
    File: src/error/error.h
    Author: Trident Apollo  
    Date: 6-02-2026
    Reference: None
    Description:
        Error code definitions and error handling utilities for Torilate.
        
        This module provides a layered error handling system where:
        - Each layer can create errors with runtime context
        - Static error message table provides base descriptions
        - Error structs propagate up the call stack
        - Context can be enriched at each layer
*/


#ifndef TORILATE_ERROR_H
#define TORILATE_ERROR_H

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include "torilate.h"


/* Error Codes */
typedef enum {
    SUCCESS = 0,

    /* CLI errors */
    ERR_NO_ARGS,
    ERR_INVALID_ARGS,
    ERR_INVALID_COMMAND,

    /* Network errors */
    ERR_NETWORK_IO,
    ERR_INVALID_ADDRESS,
    ERR_NET_RECV_FAILED,
    ERR_SOCK_INIT_FAILED,
    ERR_CONNECTION_FAILED,
    ERR_TOR_CONNECTION_FAILED,
    ERR_SOCKET_CREATION_FAILED,
    ERR_ADDRESS_RESOLUTION_FAILED,

    /* HTTP errors */
    ERR_INVALID_URI,
    ERR_BAD_RESPONSE,
    ERR_INVALID_SCHEMA,
    ERR_INVALID_HEADER,
    ERR_HTTP_REQUEST_FAILED,
    ERR_HTTP_REDIRECT_LIMIT,
    ERR_HTTP_REDIRECT_FAILED,

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


/* 
 * Error Struct
 * 
 * Represents an error with both a code and optional contextual message.
 * Can be passed by value (small enough) or by pointer.
 */
typedef struct Error {
    ErrorCode code;
    char message[512];
} Error;


/* ============================================================================
 * Error Creation Macros
 * ============================================================================
 * These macros provide convenient ways to create Error structs.
 */

/* Create a success error (no error) */
#define ERR_OK() ((Error){.code = SUCCESS, .message = {0}})

/* Create an error with just a code (uses static message table) */
#define ERR_CODE(error_code) ((Error){.code = (error_code), .message = {0}})

/* Create an error with code and formatted message */
#define ERR_NEW(error_code, ...) (err_create((error_code), __VA_ARGS__))

/* Propagate an error up the stack with additional context */
#define ERR_PROPAGATE(err, ...) (err_propagate((err), __VA_ARGS__))

/* Check if an error represents failure */
#define ERR_FAILED(err) ((err).code != SUCCESS)



/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/*
 * Get the base error message from the static table.
 * 
 * @param code      The error code
 * 
 * @returns         Static string describing the error
 */
const char *err_get_base_message(ErrorCode code);

/*
 * Get the formatted error message.
 * 
 *  @param err      Pointer to error struct (will be modified to contain formatted message)
 *  @param verbose  If true, shows full propagation chain; if false, shows only immediate error
 *  
 *  @returns        Pointer to formatted message in err->message
 * 
 * @note
 * Simple mode (verbose=false): "torilate: (code) base_msg: top_level_error_context"
 * 
 * Verbose mode (verbose=true): "torilate: (code) base_msg: context1: context2: immediate_error"
 */
const char *get_err_msg(Error *err, bool verbose);

/*
 * Create a new error with formatted message.
 * 
 *  @param code     Error code
 *  @param fmt      Printf-style format string
 *  @param ...      Format arguments
 * 
 *  @returns        Error struct with code and formatted message
 * 
 *  Example:
 *  ```
 *  return err_create(ERR_CONNECTION_FAILED, "Failed to connect to %s:%d", host, port);
 *  ```
 */
Error err_create(ErrorCode code, const char *fmt, ...);

/*
 * Propagate an error with additional context.
 * Preserves original error code but adds contextual message.
 * 
 *  @param err      Original error to propagate
 *  @param fmt      Printf-style format string for additional context
 *  @param ...      Format arguments
 *  
 *  @returns        Error struct with original code and enriched message
 * 
 *  Example:
 *  ```
 *  Error err = socks4_connect(...);
 *  if (ERR_FAILED(err)) {
 *      return err_propagate(err, "While establishing SOCKS tunnel to %s", host);
 *  }
 *  ```
 */
Error err_propagate(Error err, const char *fmt, ...);

#endif