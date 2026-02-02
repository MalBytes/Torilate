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

char *strdup(const char *s);
char *strndup(const char *s, size_t n);
int parse_uri(const char *uri, URI *out);

#endif