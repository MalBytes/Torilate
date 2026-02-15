# Error Handling Architecture Guide

## Overview

This document describes Torilate's **layered error handling architecture**, which allows errors to propagate up the call stack while accumulating contextual information at each layer.

### Key Design Principles

1. **Error Codes + Context**: Each error has both a code (for categorization) and an optional message (for runtime details)
2. **Static Message Table**: Base error descriptions remain in a lookup table for quick reference
3. **Context Enrichment**: Each layer can add specific details as errors bubble up
4. **Type Safety**: Errors are proper structs, not just integers
5. **Convenience**: Macros make error handling clean and expressive

---

## Core Concepts

### Error Structure

```c
typedef struct {
    ErrorCode code;      // What category of error
    char message[512];   // Why it happened (runtime details)
} Error;
```

### Error Lifecycle

```
Layer 3 (lowest): Creates error with specific details
    ↓
Layer 2 (middle): Adds context about what it was trying to do
    ↓
Layer 1 (highest): Adds user-facing context
    ↓
Main: Formats and displays to user
```

---

## Usage Patterns

### 1. Creating Errors (Lowest Layer)

When an error first occurs, create it with specific runtime details:

```c
// Simple error with just a code (uses static message table)
return ERR_CODE(ERR_INVALID_ADDRESS);

// Error with runtime context
return ERR_NEW(ERR_CONNECTION_FAILED, 
    "Failed to connect to %s:%d", host, port);

// Error with detailed system information
return ERR_NEW(ERR_NETWORK_IO, 
    "recv() failed with errno=%d (%s)", errno, strerror(errno));
```

### 2. Propagating Errors (Middle Layers)

When a function calls another and gets an error, it can add context:

```c
Error err = net_connect(&sock, TOR_IP, TOR_PORT);
if (ERR_FAILED(err)) {
    // Add context about what we were trying to do
    return ERR_PROPAGATE(err, "While connecting to Tor proxy");
}
```

This preserves the original error code but enriches the message:
- Original: `"Failed to connect to 127.0.0.1:9050"`
- Propagated: `"While connecting to Tor proxy: Failed to connect to 127.0.0.1:9050"`

### 3. Checking for Errors

```c
// Check if operation failed
if (ERR_FAILED(err)) {
    // Handle error
}

// Check if operation succeeded
if (ERR_SUCCESS(err)) {
    // Continue
}

// Can still check specific codes
if (err.code == ERR_NETWORK_IO) {
    // Handle network errors differently
}
```

### 4. Creating Success Values

```c
// Return success
return ERR_OK();

// Or simply
Error err = ERR_OK();
return err;
```

---

## Migration Examples

### Example 1: Simple Function (Net Layer)

**Before:**
```c
ErrorCode net_connect(NetSocket *sock, const char *ip, uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return ERR_SOCKET_CREATION_FAILED;

    // ... connection code ...
    
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return ERR_CONNECTION_FAILED;
    }

    sock->handle = (int)s;
    return SUCCESS;
}
```

**After:**
```c
Error net_connect(NetSocket *sock, const char *ip, uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        return ERR_NEW(ERR_SOCKET_CREATION_FAILED, 
            "socket() failed with error %d", WSAGetLastError());
    }

    // ... connection code ...
    
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int wsa_err = WSAGetLastError();
        closesocket(s);
        return ERR_NEW(ERR_CONNECTION_FAILED, 
            "Failed to connect to %s:%d (WSA error %d)", ip, port, wsa_err);
    }

    sock->handle = (int)s;
    return ERR_OK();
}
```

### Example 2: Function with Error Propagation (SOCKS Layer)

**Before:**
```c
ErrorCode socks4_connect(NetSocket *sock, const char *dst_ip, 
                         uint16_t dst_port, const char *user_id, 
                         NetAddrType addr_type) {
    // ... build request ...
    
    if (net_send_all(sock, request, offset) != 0)
        return ERR_NETWORK_IO;

    if (net_recv(sock, response, sizeof(response)) != sizeof(response))
        return ERR_NETWORK_IO;

    if (response[0] != 0x00 || response[1] != SOCKS4_OK)
        return ERR_CONNECTION_FAILED;

    return SUCCESS;
}
```

**After:**
```c
Error socks4_connect(NetSocket *sock, const char *dst_ip, 
                     uint16_t dst_port, const char *user_id, 
                     NetAddrType addr_type) {
    // ... build request ...
    
    Error err = net_send_all(sock, request, offset);
    if (ERR_FAILED(err)) {
        return ERR_PROPAGATE(err, "Failed to send SOCKS4 request");
    }

    err = net_recv_exact(sock, response, sizeof(response));
    if (ERR_FAILED(err)) {
        return ERR_PROPAGATE(err, "Failed to receive SOCKS4 response");
    }

    if (response[0] != 0x00 || response[1] != SOCKS4_OK) {
        return ERR_NEW(ERR_CONNECTION_FAILED, 
            "SOCKS4 request rejected: code=0x%02x (destination: %s:%d)", 
            response[1], dst_ip, dst_port);
    }

    return ERR_OK();
}
```

### Example 3: HTTP Layer with Multiple Calls

**Before:**
```c
ErrorCode http_get(NetSocket *sock, const char *host, const char *path, 
                   bool follow_redirects, int max_redirects, 
                   HttpResponse *response) {
    HttpResponse current_response;
    ErrorCode status = http_get_once(sock, host, path, &current_response);
    if (status != SUCCESS)
        return status;

    // ... redirect handling ...
    
    *response = current_response;
    return SUCCESS;
}
```

**After:**
```c
Error http_get(NetSocket *sock, const char *host, const char *path, 
               bool follow_redirects, int max_redirects, 
               HttpResponse *response) {
    HttpResponse current_response;
    Error err = http_get_once(sock, host, path, &current_response);
    if (ERR_FAILED(err)) {
        return ERR_PROPAGATE(err, "GET request to %s%s failed", host, path);
    }

    // ... redirect handling ...
    
    *response = current_response;
    return ERR_OK();
}
```

### Example 4: Main Function

**Before:**
```c
int main(int argc, char *argv[]) {
    int status = 0;
    Error error = {0};
    
    status = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (status != SUCCESS) {
        snprintf(error.message, sizeof(error.message), 
            "Cannot connect to TOR at %s:%d", TOR_IP, TOR_PORT);
        error.code = ERR_TOR_CONNECTION_FAILED;
        goto cleanUp;
    }
    
    // More operations...
    
cleanUp:
    if (error.code != SUCCESS) {
        printf("%s\n", get_err_msg(&error));
    }
    return error.code;
}
```

**After:**
```c
int main(int argc, char *argv[]) {
    Error error = ERR_OK();
    
    error = net_connect(&sock, TOR_IP, TOR_PORT); 
    if (ERR_FAILED(error)) {
        error = ERR_PROPAGATE(error, "Cannot establish Tor connection");
        goto cleanUp;
    }
    
    // More operations...
    
cleanUp:
    if (ERR_FAILED(error)) {
        printf("%s\n", get_err_msg(&error));
    }
    return error.code;
}
```

---

## Function Signature Patterns

### Pattern 1: Return Error Directly (Recommended)

```c
Error my_function(params) {
    // ... operations ...
    if (something_failed) {
        return ERR_NEW(ERR_CODE, "details");
    }
    return ERR_OK();
}
```

**Use when:** Function's primary purpose is the operation, not returning data

### Pattern 2: Return Error + Output Parameters

```c
Error my_function(params, OutputType *out) {
    // ... operations ...
    if (something_failed) {
        return ERR_NEW(ERR_CODE, "details");
    }
    *out = result;
    return ERR_OK();
}
```

**Use when:** Function needs to return both data and errors

### Pattern 3: Integer Return with Error Out-Parameter

```c
int my_function(params, Error *err) {
    // ... operations ...
    if (something_failed) {
        *err = ERR_NEW(ERR_CODE, "details");
        return -1;
    }
    *err = ERR_OK();
    return bytes_processed;
}
```

**Use when:** Function returns scalar values (like counts, sizes, file descriptors)

---

## Macros Reference

| Macro | Purpose | Example |
|-------|---------|---------|
| `ERR_OK()` | Create success value | `return ERR_OK();` |
| `ERR_CODE(code)` | Error with no custom message | `return ERR_CODE(ERR_INVALID_ARGS);` |
| `ERR_NEW(code, fmt, ...)` | Error with formatted message | `return ERR_NEW(ERR_IO, "Failed to read %s", path);` |
| `ERR_FAILED(err)` | Check if error | `if (ERR_FAILED(err)) { ... }` |
| `ERR_SUCCESS(err)` | Check if success | `if (ERR_SUCCESS(err)) { ... }` |
| `ERR_PROPAGATE(err, fmt, ...)` | Add context to error | `return ERR_PROPAGATE(err, "While parsing %s", file);` |

---

## Best Practices

### ✅ DO

1. **Add specific runtime details when creating errors**
   ```c
   return ERR_NEW(ERR_CONNECTION_FAILED, 
       "Cannot connect to %s:%d (timeout after %d seconds)", 
       host, port, timeout);
   ```

2. **Use ERR_PROPAGATE to add layer-specific context**
   ```c
   Error err = lower_function();
   if (ERR_FAILED(err)) {
       return ERR_PROPAGATE(err, "While establishing SOCKS tunnel");
   }
   ```

3. **Keep error messages concise and informative**
   - Include what failed
   - Include relevant values (IPs, ports, filenames, etc.)
   - Include system error codes when available

4. **Use the static error table messages as documentation**
   - Developers can look up `ERR_CONNECTION_FAILED` to understand the category
   - Runtime message provides the specific instance details

### ❌ DON'T

1. **Don't lose error information when propagating**
   ```c
   // BAD: Original error details lost
   Error err = lower_function();
   if (ERR_FAILED(err)) {
       return ERR_NEW(ERR_GENERIC, "Something failed");
   }
   
   // GOOD: Preserve original error
   if (ERR_FAILED(err)) {
       return ERR_PROPAGATE(err, "Context about what we were doing");
   }
   ```

2. **Don't put sensitive information in error messages**
   - Avoid passwords, tokens, full file contents
   - Error messages may be logged or displayed

3. **Don't create overly long error messages**
   - 512 bytes is the limit
   - Be specific but concise

---

## Migration Strategy

### Phase 1: Update Core Layers (Start Here)
- [ ] Error module (✅ Complete)
- [ ] Net layer (`net/*.c`)
- [ ] SOCKS layer (`socks/*.c`)

### Phase 2: Update Protocol Layers
- [ ] HTTP layer (`http/*.c`)
- [ ] Utility layer (`util/*.c`)

### Phase 3: Update Application Layer
- [ ] CLI layer (`cli/*.c`)
- [ ] Main application (`torilate.c`)

### Phase 4: Testing
- [ ] Test error messages at each layer
- [ ] Verify error propagation
- [ ] Check error message clarity

---

## Example: Full Call Stack with Error Propagation

```c
// Layer 4: System call (lowest)
Error net_send_all(NetSocket *sock, const void *data, size_t len) {
    int sent = send(sock->handle, data, len, 0);
    if (sent != len) {
        return ERR_NEW(ERR_NETWORK_IO, 
            "send() failed: sent %d/%zu bytes, errno=%d", 
            sent, len, errno);
    }
    return ERR_OK();
}

// Layer 3: Protocol implementation
Error socks4_connect(NetSocket *sock, const char *dst_ip, uint16_t dst_port, 
                     const char *user_id, NetAddrType addr_type) {
    // ... build request ...
    
    Error err = net_send_all(sock, request, offset);
    if (ERR_FAILED(err)) {
        return ERR_PROPAGATE(err, 
            "Failed to send SOCKS4 CONNECT to %s:%d", dst_ip, dst_port);
    }
    // ... rest of function ...
    return ERR_OK();
}

// Layer 2: Application protocol
Error http_get(NetSocket *sock, const char *host, const char *path, 
               bool follow_redirects, int max_redirects, HttpResponse *response) {
    Error err = http_send_request(sock, "GET", host, path);
    if (ERR_FAILED(err)) {
        return ERR_PROPAGATE(err, "GET request to http://%s%s failed", host, path);
    }
    // ... rest of function ...
    return ERR_OK();
}

// Layer 1: Main application
int main(int argc, char *argv[]) {
    Error err = http_get(&sock, "example.com", "/api/data", false, 0, &response);
    if (ERR_FAILED(err)) {
        err = ERR_PROPAGATE(err, "Failed to fetch data from API");
        printf("%s\n", get_err_msg(&err));
        return err.code;
    }
    return 0;
}
```

**Final error message seen by user:**
```
torilate: (4) Network I/O error: Failed to fetch data from API: GET request to http://example.com/api/data failed: Failed to send SOCKS4 CONNECT to 93.184.216.34:80: send() failed: sent 0/12 bytes, errno=10054
```

This shows the complete error path from lowest (system call) to highest (user action).

---

## Additional Notes

### Performance Considerations

- `Error` struct is 516 bytes (code + 512-byte message)
- Passing by value is efficient on modern systems (single memory copy)
- For very hot paths, consider passing `Error*` instead
- Most functions return infrequently enough that this isn't a concern

### Thread Safety

- Error structs are per-thread (on stack or as return values)
- Error message table is read-only and thread-safe
- No global state used in error handling

### Compatibility

- Works with C99 and later
- Compatible with both Windows and POSIX
- No external dependencies beyond standard library

---

## Quick Reference Card

```c
/* Creating errors */
return ERR_OK();                                    // Success
return ERR_CODE(ERR_INVALID_ARGS);                 // Error, no details
return ERR_NEW(ERR_IO, "Can't read %s", path);    // Error with details

/* Checking errors */
if (ERR_FAILED(err)) { /* handle */ }              // Check for error
if (ERR_SUCCESS(err)) { /* continue */ }           // Check for success
if (err.code == ERR_NETWORK_IO) { /* specific */ } // Check specific code

/* Propagating errors */
Error err = lower_function();
if (ERR_FAILED(err)) {
    return ERR_PROPAGATE(err, "Context: %s", info);
}

/* Displaying errors */
printf("%s\n", get_err_msg(&err));                 // Format for user
```

---

**Questions or need help migrating specific code? Refer to the examples above or consult the error module source code.**
