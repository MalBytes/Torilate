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
#include "error/error.h"

static int http_send(NetSocket *sock, const char *request) {
    size_t len = strlen(request);
    return net_send_all(sock, request, len);
}

static int64_t http_recv_response(NetSocket *sock, HttpResponse *out) {
    int total = 0;
    out->error_code = 0;

    while (total < HTTP_MAX_RESPONSE - 1) {
        int n = net_recv(sock, out->raw + total, HTTP_MAX_RESPONSE - 1 - total);
        if (n <= 0)
            break;

        total += n;

        // /* Stop early if server closed connection */
        // if (strstr(out->raw, "\r\n\r\n"))
        //     break;
    }

    out->raw[total] = '\0';

    /* Parse status code */
    int code;
    char *status = out->raw;
    while (*status == ' ' || *status == '\r' || *status == '\n')
        status++;

    if (sscanf(status, "HTTP/%*d.%*d %d", &code) != 1 || code < 100 || code > 599) {
        out->error_code = ERR_BAD_RESPONSE;
        return -1;
    }

    out->status_code = (HttpStatusCode)code;

    return total;
}

int64_t http_get(NetSocket *sock, const char *host, const char *path, HttpResponse *out) {
    char request[2048];

    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Torilate\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host);

    if (http_send(sock, request) != 0)
        return -1;

    return http_recv_response(sock, out);
}

int64_t http_post(NetSocket *sock, const char *host, const char *path, const char *content_type, const char *body, HttpResponse *out) {
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
    if (http_send(sock, request) != 0)
        return -1;

    return http_recv_response(sock, out);
}
