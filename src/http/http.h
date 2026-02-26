/* 
    File: src/http/http.h
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - HTTP/1.1 (RFC 7231): https://datatracker.ietf.org/doc/html/rfc7231
        - HTTP/1.1 (RFC 7230): https://datatracker.ietf.org/doc/html/rfc7230
    Description:
        Minimal HTTP/1.1 client implementation for Torilate.
        Provides basic GET and POST request functionality over
        an already-established TCP tunnel.
*/

#ifndef TORILATE_HTTP_H
#define TORILATE_HTTP_H

#include <stddef.h>
#include "torilate.h"
#include "net/socket.h"
#include "error/error.h"

#define HTTP_MAX_RESPONSE 8192


typedef enum {
    /* 1xx Informational */
    HTTP_CONTINUE                        = 100,
    HTTP_SWITCHING_PROTOCOLS             = 101,
    HTTP_PROCESSING                      = 102,
    HTTP_EARLY_HINTS                     = 103,

    /* 2xx Success */
    HTTP_OK                              = 200,
    HTTP_CREATED                         = 201,
    HTTP_ACCEPTED                        = 202,
    HTTP_NON_AUTHORITATIVE_INFORMATION   = 203,
    HTTP_NO_CONTENT                      = 204,
    HTTP_RESET_CONTENT                   = 205,
    HTTP_PARTIAL_CONTENT                 = 206,
    HTTP_MULTI_STATUS                    = 207,
    HTTP_ALREADY_REPORTED                = 208,
    HTTP_IM_USED                         = 226,

    /* 3xx Redirection */
    HTTP_MULTIPLE_CHOICES                = 300,
    HTTP_MOVED_PERMANENTLY               = 301,
    HTTP_FOUND                           = 302,
    HTTP_SEE_OTHER                       = 303,
    HTTP_NOT_MODIFIED                    = 304,
    HTTP_USE_PROXY                       = 305,
    HTTP_TEMPORARY_REDIRECT              = 307,
    HTTP_PERMANENT_REDIRECT              = 308,

    /* 4xx Client Error */
    HTTP_BAD_REQUEST                     = 400,
    HTTP_UNAUTHORIZED                    = 401,
    HTTP_PAYMENT_REQUIRED                = 402,
    HTTP_FORBIDDEN                       = 403,
    HTTP_NOT_FOUND                       = 404,
    HTTP_METHOD_NOT_ALLOWED              = 405,
    HTTP_NOT_ACCEPTABLE                  = 406,
    HTTP_PROXY_AUTHENTICATION_REQUIRED   = 407,
    HTTP_REQUEST_TIMEOUT                 = 408,
    HTTP_CONFLICT                        = 409,
    HTTP_GONE                            = 410,
    HTTP_LENGTH_REQUIRED                 = 411,
    HTTP_PRECONDITION_FAILED             = 412,
    HTTP_PAYLOAD_TOO_LARGE               = 413,
    HTTP_URI_TOO_LONG                    = 414,
    HTTP_UNSUPPORTED_MEDIA_TYPE          = 415,
    HTTP_RANGE_NOT_SATISFIABLE           = 416,
    HTTP_EXPECTATION_FAILED              = 417,
    HTTP_IM_A_TEAPOT                     = 418,
    HTTP_MISDIRECTED_REQUEST             = 421,
    HTTP_UNPROCESSABLE_ENTITY            = 422,
    HTTP_LOCKED                          = 423,
    HTTP_FAILED_DEPENDENCY               = 424,
    HTTP_TOO_EARLY                       = 425,
    HTTP_UPGRADE_REQUIRED                = 426,
    HTTP_PRECONDITION_REQUIRED           = 428,
    HTTP_TOO_MANY_REQUESTS               = 429,
    HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    HTTP_UNAVAILABLE_FOR_LEGAL_REASONS   = 451,

    /* 5xx Server Error */
    HTTP_INTERNAL_SERVER_ERROR           = 500,
    HTTP_NOT_IMPLEMENTED                 = 501,
    HTTP_BAD_GATEWAY                     = 502,
    HTTP_SERVICE_UNAVAILABLE             = 503,
    HTTP_GATEWAY_TIMEOUT                 = 504,
    HTTP_HTTP_VERSION_NOT_SUPPORTED      = 505,
    HTTP_VARIANT_ALSO_NEGOTIATES         = 506,
    HTTP_INSUFFICIENT_STORAGE            = 507,
    HTTP_LOOP_DETECTED                   = 508,
    HTTP_NOT_EXTENDED                    = 510,
    HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511
} HttpStatusCode;

typedef struct {
    uint64_t bytes_received;
    HttpStatusCode status_code;
    char raw[HTTP_MAX_RESPONSE];
} HttpResponse;


/*
 * Perform an HTTP GET request.
 *
 *  @param uri               target URI (e.g. "http://example.com/index.html")
 *  @param follow_redirects  whether to automatically follow HTTP redirects (3xx)
 *  @param max_redirects     maximum number of redirects to follow (if follow_redirects is true)
 *  @param response          HttpResponse structure to store the response
 *  @param headers           Additional HTTP headers to include in the request
 *
 *  @return ERR_OK on success and an Error struct on failure
 */
Error http_get(const char *uri,
               const char **headers,
               int headers_count,
               bool follow_redirects,
               int max_redirects,
               HttpResponse *response);

/*
 * Perform an HTTP POST request.
 *
 *  @param uri               target URI
 *  @param content_type      Content-Type header value
 *  @param body              POST body data
 *  @param follow_redirects  whether to automatically follow HTTP redirects (3xx)
 *  @param max_redirects     maximum number of redirects to follow (if follow_redirects is true)
 *  @param response          HttpResponse structure to store the response
 *  @param headers           Additional HTTP headers to include in the request
 *
 *  @return ERR_OK on success and an Error struct on failure
 */
Error http_post(const char *uri,
                const char *body,
                const char **headers,
                int headers_count,
                bool follow_redirects,
                int max_redirects,
                HttpResponse *response);

#endif
