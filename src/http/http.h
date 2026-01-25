/* 
    File: http.h
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - HTTP/1.1: RFC 7230, RFC 7231
          https://datatracker.ietf.org/doc/html/rfc7230
          https://datatracker.ietf.org/doc/html/rfc7231
    Description:
        Minimal HTTP/1.1 client implementation for Torilate.
        Provides basic GET and POST request functionality over
        an already-established TCP tunnel.
*/

#ifndef TORILATE_HTTP_H
#define TORILATE_HTTP_H

#include <stddef.h>
#include "net/socket.h"

#define HTTP_MAX_RESPONSE 8192

typedef struct {
    int status_code;
    char raw[HTTP_MAX_RESPONSE];
} HttpResponse;


/*
 * Perform an HTTP GET request.
 *
 * Parameters:
 *   sock     - connected socket (already tunneled via SOCKS)
 *   host     - target host (e.g. "example.com")
 *   path     - request path (e.g. "/index.html")
 *   response - HttpResponse structure to store the response
 *
 * Returns:
 *   number of bytes received on success
 *  -1 on error
 */
int http_get(NetSocket *sock,
             const char *host,
             const char *path,
             HttpResponse *response);

/*
 * Perform an HTTP POST request.
 *
 * Parameters:
 *   sock     - connected socket
 *   host     - target host
 *   path     - request path
 *   body     - POST body data
 *   response - HttpResponse structure to store the response
 *
 * Returns:
 *   number of bytes received on success
 *  -1 on error
 */
int http_post(NetSocket *sock,
              const char *host,
              const char *path,
              const char *body,
              HttpResponse *response);

#endif /* TORILATE_HTTP_H */
