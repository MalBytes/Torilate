/*
    File: src/util/util.c
    Author: Trident Apollo  
    Date: 01-02-2026
    Reference: None
    Description:
        Common utility function implementations for Torilate.
*/


#include "util/util.h"
#include "error/error.h"


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

int parse_http_response(HttpResponse *response, char *out, size_t out_size, size_t *resp_size, bool raw) {
    int status_code = 0;
    char status_text[64] = {0};
    int content_length = -1;
    
    // If raw flag is set, just copy the response as-is
    if (raw) {
        const char *start = strstr(response->raw, "HTTP");
        if (!start) {
            return ERR_BAD_RESPONSE;
        }

        const char *end = response->raw + response->bytes_received;
        while (end > start && isspace((unsigned char)end[-1])) {
            end--;
        }

        size_t len = (size_t)(end - start);
        if (len >= out_size) {
            len = out_size - 1;
        }

        memcpy(out, start, len);
        out[len] = '\0';

        if (resp_size) {
            *resp_size = len;
        }

        return SUCCESS;
    }

    /* Parse status line */
    const char *status_line = strstr(response->raw, "HTTP");
    if (sscanf(status_line, "HTTP/%*s %d %63[^\r\n]", &status_code, status_text) != 2 ||
        status_code < 100 || status_code > 599) {
        return ERR_BAD_RESPONSE;
    }

    /* Find end of headers */
    const char *header_end = strstr(response->raw, "\r\n\r\n");
    if (!header_end) {
        return ERR_BAD_RESPONSE;
    }
    header_end += 4;

    /* Find Content-Length */
    const char *cl = strstr(response->raw, "Content-Length:");
    if (cl) {
        sscanf(cl, "Content-Length: %d", &content_length);
    }

    /* Format output header */
    int written;
    int body_length;
    if (content_length >= 0) {
        written = snprintf(
            out,
            out_size,
            "Status Code: %d\n"
            "Status Description: %s\n"
            "Content Length: %d\n\n",
            status_code,
            status_text,
            content_length
        );
        body_length = content_length;
    } else {
        written = snprintf(
            out,
            out_size,
            "Status Code: %d\n"
            "Status Description: %s\n\n",
            status_code,
            status_text
        );
        size_t len = strlen(header_end);
        if (len < 4) {
            return ERR_BAD_RESPONSE;
        }
        body_length = strlen(header_end) - 4; /* -4 to exclude the trailing \r\n\r\n */
    }

    if (written < 0 || (size_t)written >= out_size) {
        return ERR_IO;
    }

    /* Copy body safely */
    size_t remaining = out_size - written - 1;
    if ((size_t)body_length > remaining) {
        body_length = (int)remaining;
    }

    memcpy(out + written, header_end, body_length);
    out[written + body_length] = '\0';
    *resp_size = written + body_length;

    return SUCCESS;
}
