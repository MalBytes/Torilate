/* 
    File: src/http/http.c
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - HTTP/1.1 (RFC 7231): https://datatracker.ietf.org/doc/html/rfc7231
        - HTTP/1.1 (RFC 7230): https://datatracker.ietf.org/doc/html/rfc7230
    Description:
        Minimal HTTP/1.1 request implementation (GET, POST)
        for use over Torilate network tunnels.
*/

#include "http/http.h"
#include "util/util.h"


/* Function Prototypes*/
static Error http_send(NetSocket *sock, const char *request);
static Error http_recv_response(NetSocket *sock, HttpResponse *out);

/* Public API */
Error http_get(NetSocket *sock, const char *host, const char *path, HttpResponse *out) {
    char request[2048];

    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Torilate\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host);

    Error err = http_send(sock, request);
    if (ERR_FAILED(err))
        return err;

    return http_recv_response(sock, out);
}

Error http_post(NetSocket *sock, const char *host, const char *path, const char *content_type, const char *body, HttpResponse *out) {
    char request[4096];
    size_t body_len = body ? strlen(body) : 0;
    
    char content_type_header[256];
    snprintf(content_type_header, 256, 
             "%s%s%s", 
             content_type ? "Content-Type: " : "",
             content_type ? content_type : "",
             content_type ? "\r\n" : "");

    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Torilate\r\n"
             "%s"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n%s",
             path, host, content_type_header, body_len, body ? body : "");

    Error err = http_send(sock, request);
    if (ERR_FAILED(err))
        return err;

    return http_recv_response(sock, out);
}

static Error http_send(NetSocket *sock, const char *request) {
    size_t len = strlen(request);
    return net_send_all(sock, request, len);
}

static Error http_recv_response(NetSocket *sock, HttpResponse *out) {
    int total = 0;
    out->bytes_received = 0;

    while (total < HTTP_MAX_RESPONSE - 1) {
        size_t bytes_received = 0;
        Error err = net_recv(sock, out->raw + total, HTTP_MAX_RESPONSE - 1 - total, &bytes_received);

        if (ERR_FAILED(err))
            return ERR_PROPAGATE(err, "Failed to receive HTTP response");
        if (bytes_received == 0)
            break;

        total += bytes_received;
    }

    out->raw[total] = '\0';

    /* Parse status code */
    int code;
    char *status = out->raw;
    while (*status == ' ' || *status == '\r' || *status == '\n')
        status++;

    if (sscanf(status, "HTTP/%*d.%*d %d", &code) != 1 || code < 100 || code > 599) {
        return ERR_NEW(ERR_BAD_RESPONSE, "Malformed HTTP header: Unable to parse status code");
    }

    out->status_code = (HttpStatusCode)code;
    out->bytes_received = total;

    return ERR_OK();
}