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
char *strdup(const char *s);
char *strndup(const char *s, size_t n);

// Parsing utilities
int parse_uri(const char *uri, URI *out);

// File handling utilities
int write_to(const char *file_name, const char *data, size_t len);
int read_from(const char *file_name, char **buffer, size_t *out_len);

#endif