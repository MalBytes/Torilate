# Example: Refactoring the Net Layer to Use Error Structs

This file demonstrates a practical refactoring of the network layer to use the new Error-based error handling system.

## Current Implementation (Before)

### socket.h (Current)
```c
/* Connection */
int net_connect(NetSocket *sock, const char *ip, uint16_t port);

/* I/O */
int net_send_all(NetSocket *sock, const void *buf, size_t len);
int net_recv(NetSocket *sock, void *buf, size_t len);
```

### socket_win32.c (Current)
```c
int net_connect(NetSocket *sock, const char *ip, uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        closesocket(s);
        return -1;
    }

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return -1;
    }

    sock->handle = (int)s;
    return 0;
}

int net_send_all(NetSocket *sock, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *ptr = (const char*)buf;

    while (total_sent < len) {
        int sent = send((SOCKET)sock->handle, ptr + total_sent, 
                       (int)(len - total_sent), 0);
        if (sent == SOCKET_ERROR)
            return -1;
        if (sent == 0)
            return -1;

        total_sent += sent;
    }

    return 0;
}

int net_recv(NetSocket *sock, void *buf, size_t len) {
    int received = recv((SOCKET)sock->handle, (char*)buf, (int)len, 0);
    if (received == SOCKET_ERROR)
        return -1;
    return received;
}
```

## New Implementation (After)

### socket.h (New - with Error structs)
```c
#include "error/error.h"

/* Connection */
Error net_connect(NetSocket *sock, const char *ip, uint16_t port);

/* I/O */
Error net_send_all(NetSocket *sock, const void *buf, size_t len);

// Returns bytes received via out parameter, Error for status
Error net_recv(NetSocket *sock, void *buf, size_t len, int *bytes_received);

// Alternative: Keep int return for bytes, use out parameter for error
// int net_recv(NetSocket *sock, void *buf, size_t len, Error *err);
```

### socket_win32.c (New - with detailed error messages)
```c
#include "net/socket.h"
#include "error/error.h"
#include <winsock2.h>
#include <ws2tcpip.h>

Error net_connect(NetSocket *sock, const char *ip, uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        int wsa_err = WSAGetLastError();
        return ERR_NEW(ERR_SOCKET_CREATION_FAILED, 
            "socket() failed with WSA error %d", wsa_err);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        closesocket(s);
        return ERR_NEW(ERR_INVALID_ADDRESS, 
            "Invalid IPv4 address: %s", ip);
    }

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int wsa_err = WSAGetLastError();
        closesocket(s);
        return ERR_NEW(ERR_CONNECTION_FAILED, 
            "Failed to connect to %s:%d (WSA error %d)", 
            ip, port, wsa_err);
    }

    sock->handle = (int)s;
    return ERR_OK();
}

Error net_send_all(NetSocket *sock, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *ptr = (const char*)buf;

    while (total_sent < len) {
        int sent = send((SOCKET)sock->handle, ptr + total_sent, 
                       (int)(len - total_sent), 0);
        
        if (sent == SOCKET_ERROR) {
            int wsa_err = WSAGetLastError();
            return ERR_NEW(ERR_NETWORK_IO, 
                "send() failed after %zu/%zu bytes (WSA error %d)", 
                total_sent, len, wsa_err);
        }
        
        if (sent == 0) {
            return ERR_NEW(ERR_NETWORK_IO, 
                "Connection closed by peer after %zu/%zu bytes sent", 
                total_sent, len);
        }

        total_sent += sent;
    }

    return ERR_OK();
}

// Option 1: Error return, bytes via out parameter
Error net_recv(NetSocket *sock, void *buf, size_t len, int *bytes_received) {
    int received = recv((SOCKET)sock->handle, (char*)buf, (int)len, 0);
    
    if (received == SOCKET_ERROR) {
        int wsa_err = WSAGetLastError();
        *bytes_received = 0;
        return ERR_NEW(ERR_NETWORK_IO, 
            "recv() failed (WSA error %d)", wsa_err);
    }
    
    if (received == 0) {
        *bytes_received = 0;
        return ERR_NEW(ERR_NETWORK_IO, 
            "Connection closed by peer");
    }
    
    *bytes_received = received;
    return ERR_OK();
}

// Option 2: Helper for "receive exact" - common pattern
Error net_recv_exact(NetSocket *sock, void *buf, size_t len) {
    int bytes_received;
    Error err = net_recv(sock, buf, len, &bytes_received);
    
    if (ERR_FAILED(err)) {
        return err;
    }
    
    if ((size_t)bytes_received != len) {
        return ERR_NEW(ERR_NETWORK_IO, 
            "Expected %zu bytes but received %d", len, bytes_received);
    }
    
    return ERR_OK();
}
```

## Usage Comparison

### SOCKS Layer - Before
```c
ErrorCode socks4_connect(NetSocket *sock, const char *dst_ip, 
                         uint16_t dst_port, const char *user_id, 
                         NetAddrType addr_type) {
    uint8_t request[512];
    uint8_t response[8];
    size_t  offset = 0;

    // ... build request ...

    if (net_send_all(sock, request, offset) != 0)
        return ERR_NETWORK_IO;  // Lost: What data? How many bytes? Why?

    if (net_recv(sock, response, sizeof(response)) != sizeof(response))
        return ERR_NETWORK_IO;  // Lost: How many received? Connection closed or error?

    if (response[0] != 0x00 || response[1] != SOCKS4_OK)
        return ERR_CONNECTION_FAILED;  // Lost: What was the actual response code?

    return SUCCESS;
}
```

### SOCKS Layer - After
```c
Error socks4_connect(NetSocket *sock, const char *dst_ip, 
                     uint16_t dst_port, const char *user_id, 
                     NetAddrType addr_type) {
    uint8_t request[512];
    uint8_t response[8];
    size_t  offset = 0;

    // ... build request ...

    Error err = net_send_all(sock, request, offset);
    if (ERR_FAILED(err)) {
        // Preserves: bytes sent, WSA error, etc.
        return ERR_PROPAGATE(err, 
            "Failed to send SOCKS4 CONNECT request (%zu bytes)", offset);
    }

    err = net_recv_exact(sock, response, sizeof(response));
    if (ERR_FAILED(err)) {
        // Preserves: bytes received, why it failed
        return ERR_PROPAGATE(err, 
            "Failed to receive SOCKS4 response");
    }

    if (response[0] != 0x00 || response[1] != SOCKS4_OK) {
        return ERR_NEW(ERR_CONNECTION_FAILED, 
            "SOCKS4 request rejected (VN=%d, CD=%d) for %s:%d", 
            response[0], response[1], dst_ip, dst_port);
    }

    return ERR_OK();
}
```

### Main - Before
```c
int main(int argc, char *argv[]) {
    int status = 0;
    Error error = {0};
    NetSocket sock = INVALID_SOCKET;

    net_init();
    status = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (status != SUCCESS) {
        // Manual message construction
        snprintf(error.message, sizeof(error.message), 
            "Cannot connect to TOR at %s:%d", TOR_IP, TOR_PORT);
        error.code = ERR_TOR_CONNECTION_FAILED;
        goto cleanUp;
    }

    status = socks4_connect(&sock, args.uri.host, 
                           (uint16_t)args.uri.port, 
                           PROG_NAME, args.uri.addr_type);
    if (status != SUCCESS) {
        // Manual message construction, lost lower-layer details
        snprintf(error.message, sizeof(error.message), 
            "SOCKS4 connection to %s:%d failed", 
            args.uri.host, args.uri.port);
        error.code = ERR_CONNECTION_FAILED;
        goto cleanUp;
    }

cleanUp:
    if (error.code != SUCCESS) {
        printf("%s\n", get_err_msg(&error));
    }
    return error.code;
}
```

### Main - After
```c
int main(int argc, char *argv[]) {
    Error error = ERR_OK();
    NetSocket sock = INVALID_SOCKET;

    net_init();
    
    error = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (ERR_FAILED(error)) {
        // Automatic: includes IP, port, WSA error from net_connect
        error = ERR_PROPAGATE(error, "Cannot establish Tor connection");
        goto cleanUp;
    }

    error = socks4_connect(&sock, args.uri.host, 
                          (uint16_t)args.uri.port, 
                          PROG_NAME, args.uri.addr_type);
    if (ERR_FAILED(error)) {
        // Automatic: includes full error chain from net layer -> socks layer
        error = ERR_PROPAGATE(error, 
            "SOCKS tunnel establishment failed");
        goto cleanUp;
    }

cleanUp:
    if (ERR_FAILED(error)) {
        // Now prints complete error chain:
        // "torilate: (7) Failed to connect to host: SOCKS tunnel establishment failed: 
        //  Failed to send SOCKS4 CONNECT request (12 bytes): 
        //  send() failed after 0/12 bytes (WSA error 10054)"
        printf("%s\n", get_err_msg(&error));
    }
    return error.code;
}
```

## Benefits of New Approach

### 1. Preserved Context
- **Before**: "Network I/O error" (What network operation? Where?)
- **After**: "send() failed after 0/12 bytes (WSA error 10054) while sending SOCKS4 CONNECT request (12 bytes) during SOCKS tunnel establishment"

### 2. Easier Debugging
```
torilate: (4) Network I/O error: SOCKS tunnel establishment failed: Failed to send SOCKS4 CONNECT request (12 bytes): send() failed after 0/12 bytes (WSA error 10054)
```

You can immediately see:
- What user action failed (SOCKS tunnel)
- What protocol operation failed (CONNECT request)
- What system call failed (send)
- Why it failed (WSA 10054 = Connection reset)
- How much data was involved (0/12 bytes sent)

### 3. Better Error Messages for Users
Users get actionable information:
- "Cannot connect to 127.0.0.1:9050" → Check if Tor is running
- "WSA error 10061" → Connection refused, check firewall
- "Invalid IPv4 address: not.an.ip" → Fix configuration

### 4. Consistent Pattern
Every function follows the same pattern:
1. Call lower-layer function
2. Check if it failed
3. Either propagate with context OR create new error
4. Return

### 5. Type Safety
- Can't accidentally ignore an error (would need to explicitly ignore the struct)
- Can't accidentally return wrong error code
- Compiler helps catch mistakes

## Migration Checklist for Net Layer

- [ ] Update `socket.h` function signatures
- [ ] Refactor `net_connect()` in platform-specific files
- [ ] Refactor `net_send_all()` in platform-specific files  
- [ ] Refactor `net_recv()` in platform-specific files
- [ ] Update all callers in SOCKS layer
- [ ] Update all callers in HTTP layer
- [ ] Update all callers in main application
- [ ] Test error paths thoroughly
- [ ] Verify error messages are helpful

## Testing Error Messages

After migration, test common failure scenarios:

```bash
# Tor not running
./torilate get http://example.com
# Expected: Clear message about Tor connection failure with port 9050

# Invalid port
./torilate get http://example.com:99999
# Expected: Message about invalid port number

# DNS failure (with wrong hostname)
./torilate get http://thisdoesnotexist123456789.com
# Expected: SOCKS4a failure or DNS resolution error
```

Each error should tell you:
1. What you were trying to do (user perspective)
2. Why it failed (technical details)
3. (Optionally) What to do about it
