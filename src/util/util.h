/*
    File: src/util/util.h
    Author: Trident Apollo  
    Date: 01-02-2026
    Reference: None
    Description:
        Utility functions and definitions for Torilate.
*/

#ifndef TORILATE_UTIL_H
#define TORILATE_UTIL_H

#include "torilate.h"
#include "http/http.h"
#include "error/error.h"
#include "socks/socks4.h"

typedef struct {
    int port;
    Schema schema;
    const char *host;
    const char *body;
    const char *header;
    const char *endpoint;
    NetAddrType addr_type;
} URI;

// Memory management utilities
char *ut_strdup(const char *s);
char *ut_strndup(const char *s, size_t n);

// Parsing utilities
ErrorCode parse_uri(const char *uri, URI *out);
ErrorCode parse_http_response(HttpResponse *response, char *out, size_t out_size, size_t *resp_size, bool raw);

// File handling utilities
ErrorCode write_to(const char *file_name, const char *data, size_t len);
ErrorCode read_from(const char *file_name, char **buffer, size_t *out_len);

#endif