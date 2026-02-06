/*
    File: src/util/util.c
    Author: Trident Apollo  
    Date: 01-02-2026
    Reference: None
    Description:
        Common utility function implementations for Torilate.
*/

#include <string.h>
#include <stdlib.h>
#include "util/util.h"


char *ut_strdup(const char *s) {
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p != NULL) {
        memcpy(p, s, size);
    }
    return p;
}

char *ut_strndup(const char *s, size_t n) {
    char *p;
    size_t n1;

    for (n1 = 0; n1 < n && s[n1] != '\0'; n1++)
        continue;
    p = malloc(n + 1);
    if (p != NULL) {
        memcpy(p, s, n1);
        p[n1] = '\0';
    }
    return p;
}

int parse_uri(const char *uri, URI *out) {
    char *host_start;
    char *port_start;
    char *path_start;
    
    // Determine schema
    if (strncmp(uri, "http://", 7) == 0) {
        out->schema = HTTP;
        host_start = (char *)uri + 7;
    } else if (strncmp(uri, "https://", 8) == 0) {
        out->schema = HTTPS;
        host_start = (char *)uri + 8;
    } else {
        char *format = strstr(uri, "://");
        if (format != NULL) {
            out->schema = INVALID_SCHEMA;
            out->host = ut_strndup(uri, format - uri);
            return ERR_INVALID_URI;
        }
        out->schema = HTTP; // Default to HTTP
        host_start = (char *)uri;
    }

    // Extract host and endpoint
    path_start = strchr(host_start, '/');
    if (path_start) {
        out->host = ut_strndup(host_start, path_start - host_start);
        out->endpoint = ut_strdup(path_start);
    } else {
        out->host = ut_strdup(host_start);
        out->endpoint = ut_strdup("/");
    }

    // Check for port declaration
    port_start = strchr(out->host, ':');
    if (port_start) {
        *port_start = '\0';
        out->port = atoi(port_start + 1);
    } else {
        // Default port based on schema
        out->port = (out->schema == HTTP) ? 80 : 443;
    }

    // Determine address type
    out->addr_type = net_get_addr_type(out->host);

    return SUCCESS;
}
