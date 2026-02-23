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
static Error http_get_once(NetSocket *sock, const char *host, const char *path, int port, NetAddrType addr_type, HttpResponse *out);
static Error http_post_once(NetSocket *sock, const char *host, const char *path, int port, NetAddrType addr_type, const char *content_type, const char *body, HttpResponse *out);

/* Public API */
Error http_get(const char *uri, bool follow_redirects, int max_redirects, HttpResponse *response) {
    NetSocket sock;
    URI parsed_uri;
    Error err = ERR_OK();
    HttpResponse current_response;

    err = parse_uri(uri, &parsed_uri);
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "Failed to parse URI: %s", uri);
        goto exit_get;
    }

    err = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "Cannot connect to TOR at %s:%d", TOR_IP, TOR_PORT);
        goto exit_get;
    }

    err = http_get_once(&sock, parsed_uri.host, parsed_uri.path, parsed_uri.port, parsed_uri.addr_type, &current_response);
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "Failed to get HTTP response from %s:%d", parsed_uri.host, parsed_uri.port);
        goto exit_get;
    }
        
    if (follow_redirects) {
        int redirects_followed = 0;
        while (current_response.status_code >= 300 && current_response.status_code < 400) {
            if (redirects_followed >= max_redirects) {
                err = ERR_NEW(ERR_HTTP_REDIRECT_LIMIT, "Exceeded maximum redirect limit of %d", max_redirects);
                goto exit_get;
            }
            redirects_followed++;

            // Extract Location header
            size_t url_len = 0;
            char *url = NULL;
            char *headers = strstr(current_response.raw, "\r\n");

            while (headers) {
                headers += 2;
                if (strncasecmp(headers, "Location:", 9) == 0) {
                    url = headers + 9;
                    while (*url == ' ') url++;
                    char *end = strstr(url, "\r\n");
                    if (!end) {
                        err = ERR_NEW(ERR_HTTP_REDIRECT_FAILED, "Failed to extract Location header");
                        goto exit_get;
                    }
                    url_len = end - url;
                    break;
                }
                headers = strstr(headers, "\r\n");
            }

            if (!url) {
                err = ERR_NEW(ERR_HTTP_REDIRECT_FAILED, "Redirect missing Location header");
                goto exit_get;
            }

            // Initialize new socket connection for redirect
            net_close(&sock);
            err = net_connect(&sock, TOR_IP, TOR_PORT); 
            if (ERR_FAILED(err)) {
                err = ERR_PROPAGATE(err, "Cannot connect to TOR at %s:%d", TOR_IP, TOR_PORT);
                goto exit_get;
            }

            // Check if ulr is absolute or relative
            if (url[0] == '/') {
                char new_path[1024];
                memcpy(new_path, url, url_len);
                new_path[url_len] = '\0';

                err = http_get_once(&sock, parsed_uri.host, new_path, parsed_uri.port, parsed_uri.addr_type, &current_response);
                if (ERR_FAILED(err))
                    goto exit_get;
            } else {
                char new_url[1536];
                memcpy(new_url, url, url_len);
                new_url[url_len] = '\0';

                err = parse_uri(new_url, &parsed_uri);
                if (ERR_FAILED(err)){
                    err = ERR_PROPAGATE(err, "Failed to parse redirect URL: %s", new_url);
                    goto exit_get;
                }
                    
                err = http_get_once(&sock, parsed_uri.host, parsed_uri.path, parsed_uri.port, parsed_uri.addr_type, &current_response);
                if (ERR_FAILED(err)) {    
                    err = ERR_PROPAGATE(err, "HTTP redirect failed to %s:%d", parsed_uri.host, parsed_uri.port);
                    goto exit_get;
                }
            }

        } 
    }

    memcpy(response, &current_response, sizeof(HttpResponse));
exit_get:
    net_close(&sock);
    cleanup_uri(&parsed_uri);

    return err;
}

Error http_post(const char *uri, const char *content_type, const char *body, bool follow_redirects, int max_redirects, HttpResponse *response) {
    NetSocket sock;
    URI parsed_uri;
    bool use_post = true;
    Error err = ERR_OK();
    HttpResponse current_response;

    err = parse_uri(uri, &parsed_uri);
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "Failed to parse URI: %s", uri);
        goto exit_post;
    }

    err = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "Cannot connect to TOR at %s:%d", TOR_IP, TOR_PORT);
        goto exit_post;
    }

    err = http_post_once(&sock, parsed_uri.host, parsed_uri.path, parsed_uri.port, parsed_uri.addr_type, content_type, body, &current_response);
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "Failed to POST to %s:%d", parsed_uri.host, parsed_uri.port);
        goto exit_post;
    }

    if (follow_redirects) {
        int redirects_followed = 0;

        while (current_response.status_code >= 300 && current_response.status_code < 400) {
            if (redirects_followed >= max_redirects) {
                err = ERR_NEW(ERR_HTTP_REDIRECT_LIMIT, "Exceeded maximum redirect limit of %d", max_redirects);
                goto exit_post;
            }
            redirects_followed++;

            /* Extract Location header */
            size_t url_len = 0;
            char *url = NULL;
            char *headers = strstr(current_response.raw, "\r\n");

            while (headers) {
                headers += 2;

                if (strncasecmp(headers, "Location:", 9) == 0) {
                    url = headers + 9;
                    while (*url == ' ') url++;

                    char *end = strstr(url, "\r\n");
                    if (!end) {
                        err = ERR_NEW(ERR_HTTP_REDIRECT_FAILED, "Failed to extract Location header");
                        goto exit_post;
                    }

                    url_len = end - url;
                    break;
                }

                headers = strstr(headers, "\r\n");
            }

            if (!url) {
                err = ERR_NEW(ERR_HTTP_REDIRECT_FAILED, "Redirect missing Location header");
                goto exit_post;
            }

            // Initialize new socket connection for redirect
            net_close(&sock);
            err = net_connect(&sock, TOR_IP, TOR_PORT); 
            if (ERR_FAILED(err)) {
                err = ERR_PROPAGATE(err, "Cannot connect to TOR at %s:%d", TOR_IP, TOR_PORT);
                goto exit_post;
            }

            /* Method conversion logic */
            int code = current_response.status_code;

            if (code == 301 || code == 302 || code == 303) {
                use_post = false;  // Convert to GET
            }

            /* Resolve new target */
            if (url[0] == '/') {
                char new_path[1024];
                memcpy(new_path, url, url_len);
                new_path[url_len] = '\0';

                if (use_post) {
                    err = http_post_once(&sock, parsed_uri.host, new_path, parsed_uri.port, parsed_uri.addr_type, content_type, body, &current_response);
                } else {
                    err = http_get_once(&sock, parsed_uri.host, new_path, parsed_uri.port, parsed_uri.addr_type, &current_response);
                }
            } else {
                char new_url[1536];
                memcpy(new_url, url, url_len);
                new_url[url_len] = '\0';

                cleanup_uri(&parsed_uri);

                err = parse_uri(new_url, &parsed_uri);
                if (ERR_FAILED(err)) {
                    err = ERR_PROPAGATE(err,
                                        "Failed to parse redirect URL: %s",
                                        new_url);
                    goto exit_post;
                }

                if (use_post) {
                    err = http_post_once(&sock, parsed_uri.host, parsed_uri.path, parsed_uri.port, parsed_uri.addr_type, content_type, body, &current_response);
                } else {
                    err = http_get_once(&sock, parsed_uri.host, parsed_uri.path, parsed_uri.port, parsed_uri.addr_type, &current_response);
                }
            }

            if (ERR_FAILED(err))
                goto exit_post;
        }
    }

    memcpy(response, &current_response, sizeof(HttpResponse));

exit_post:
    net_close(&sock);
    cleanup_uri(&parsed_uri);

    return err;
}

/* Internal helper functions */
static Error http_get_once(NetSocket *sock, const char *host, const char *path, int port, NetAddrType addr_type, HttpResponse *out) {
    // Establish SOCKS4 connection
    Error err;
    err = socks4_connect(sock, host, (uint16_t)port, PROG_NAME, addr_type);
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "SOCKS4 connection to %s:%d failed", host, port);
        return err;
    }

    // Construct HTTP GET request
    char request[2048];
    char port_part[16] = "";
    
    if (port != 80) {
        snprintf(port_part, sizeof(port_part), ":%d", port);
    }

    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s%s\r\n"
             "User-Agent: Torilate\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host, port_part);

    err = http_send(sock, request);
    if (ERR_FAILED(err))
        return err;

    return http_recv_response(sock, out);
}

static Error http_post_once(NetSocket *sock, const char *host, const char *path, int port, NetAddrType addr_type, const char *content_type, const char *body, HttpResponse *out) {
    // Establish SOCKS4 connection
    Error err;
    err = socks4_connect(sock, host, (uint16_t)port, PROG_NAME, addr_type);
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "SOCKS4 connection to %s:%d failed", host, port);
        return err;
    }

    // Construct HTTP POST request
    char request[4096];
    char port_part[16] = "";
    char content_type_header[256];
    size_t body_len = body ? strlen(body) : 0;
    
    if (port != 80) {
        snprintf(port_part, sizeof(port_part), ":%d", port);
    }
    
    snprintf(content_type_header, 256, 
             "%s%s%s", 
             content_type ? "Content-Type: " : "",
             content_type ? content_type : "",
             content_type ? "\r\n" : "");

    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s%s\r\n"
             "User-Agent: Torilate\r\n"
             "%s"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n%s",
             path, host, port_part, content_type_header, body_len, body ? body : "");

    err = http_send(sock, request);
    if (ERR_FAILED(err))
        return err;
    
    // Receive response
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