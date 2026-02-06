
# Torilate — Architecture Overview

This document provides a comprehensive architectural overview of **Torilate**, a command-line networking tool designed to route application-layer requests through the Tor network using SOCKS-based proxying. It is intended to equip contributors and maintainers with a clear mental model of the system from day one.

Torilate follows a **layered, protocol-oriented architecture**, emphasizing correctness, portability, and strict separation of concerns. Each layer has a single responsibility and depends only on the layer directly below it.

---

## 1. Project Structure

The codebase is organized by **architectural responsibility**, not by feature categories. Each directory represents a distinct layer or concern.

```
[Project Root]/
|
├── bin/                    # Compiled binaries
├── build/                  # CMake build artifacts
├── lib/
│   └── argtable3/          # Third-party CLI argument parsing library
│       ├── argtable3.c
│       └── argtable3.h
│
├── src/
│   ├── cli/                # Command-line interface and argument handling
│   │   ├── cli.c
│   │   └── cli.h
│   │
│   ├── error/               # Error codes and handling utilities
│   │   ├── error.c
│   │   └── error.h
│   │
│   ├── http/               # HTTP/1.1 client implementation (plaintext)
│   │   ├── http.c
│   │   └── http.h
│   │
│   ├── net/                # OS-independent networking abstraction
│   │   ├── socket.h
│   │   ├── socket_win32.c
│   │   └── socket_posix.c
│   │
│   ├── socks/              # SOCKS proxy protocol implementations
│   │   ├── socks4.c
│   │   └── socks4.h
│   │
│   ├── util/               # Shared helper utilities
│   │   ├── util.c
│   │   └── util.h
│   │
│   ├── torilate.c          # Application entry point and orchestration logic
│   └── torilate.h          # High-level shared definitions
│
├── .gitignore
├── ARCHITECTURE.md         # This document
├── CMakeLists.txt          # Build configuration
├── CODE_OF_CONDUCT.md      # Code of conduct 
├── COMMIT_GUIDLINES.md     # Commit guidlines for contribution
├── CONTRIBUTING.md         # Contribution guidelines
├── LICENSE.md              # Copyright and distribution license
└── README.md               # Readme

```

---

## 2. High-Level System Flow

Torilate operates as a **client-side request dispatcher** that establishes Tor-backed TCP tunnels and executes protocol-specific requests over them.

```
[User CLI]
     ↓
[CLI Layer]
     ↓
[Utility Layer]  (shared helpers)
     ↓
[HTTP Layer]  (plaintext)
     ↓
[SOCKS Layer] (SOCKS4 / SOCKS4a)
     ↓
[Net Layer]   (portable socket abstraction)
     ↓
[OS Networking API]
     ↓
[Tor SOCKS Proxy]
     ↓
[Tor Exit Node]
     ↓
[Target Server]
```

Each layer:

* has a single responsibility
* exposes a narrow interface
* avoids leaking implementation details upward

---

## 3. Core Components

### 3.1. CLI Layer (`src/cli`)

**Responsibility**

* Parse command-line arguments
* Validate user input
* Translate CLI intent into high-level actions

**Details**

* Uses `argtable3` for argument parsing
* Produces structured inputs for the application layer

---


## 3.2. Utility Layer (`src/util`)

**Responsibility**

* Provide **shared, low-level helper functionality**
* Centralize common operations that do **not belong to any protocol or OS layer**
* Prevent code duplication across layers

**Design Intent**

The utility layer exists to support **multiple architectural layers** without creating cross-layer coupling.
It contains **pure helper logic** with no knowledge of:

* networking protocols
* sockets
* operating system APIs
* application control flow

This makes it safe to depend on from **any upper layer**.

**Layering Rules**

The Utility layer must **not depend on**:

* HTTP logic
* SOCKS logic
* OS networking APIs
* Application orchestration

This keeps it **pure, reusable, and testable**.

---

### 3.3. HTTP Layer (`src/http`)

**Responsibility**

* Construct and send HTTP/1.1 GET and POST requests
* Receive and minimally parse HTTP responses
* Operate over an already-established TCP tunnel

**Current Capabilities**

* HTTP/1.1 (plaintext only)
* GET and POST
* Status code extraction
* Raw response buffering

**Limitations (by design)**

* No TLS (HTTPS not supported yet)
* No redirect following
* No chunked decoding
* No compression handling

---

### 3.4. SOCKS Layer (`src/socks`)

**Responsibility**

* Implement SOCKS4A proxy protocols
* Establish TCP tunnels via a TOR

**Supported Protocols**

* SOCKS4
* SOCKS4a (hostname resolution via proxy)

**Key Properties**

* RFC-faithful serialization
* PProxy side DNS resolution for SOCKS4a
* Operates purely on bytes and buffers

---

### 3.5. Net Layer (`src/net`)

**Responsibility**

* Abstract OS-specific networking APIs
* Provide a portable socket interface
* Handle byte-order conversion and address parsing

**Platform Support**

* Windows (Winsock2)
* POSIX (Linux / Unix)

**Key Properties**

* Owns Winsock initialization and cleanup
* Exposes a minimal, consistent API to upper layers
* Prevents protocol layers from depending on OS headers

---

## 3.6. Error Handling Layer (`src/error`)

**Responsibility**

* Define shared error codes
* Provide consistent, human-readable error messages

**Details**

* Exposes a central `Error` structure containing:

  * a numeric error code
  * an optional contextual message
* Maps error codes to canonical descriptions
* Formats errors for user-facing output

The Error layer is **dependency-free** and may be used by any component without introducing cross-layer coupling.

---

### 3.7. Application Entry Point (`src/torilate.c`)

**Responsibility**

* Initialize and shut down subsystems
* Coordinate CLI → protocol execution
* Manage application lifecycle

**Notes**

* Contains no protocol logic
* Contains no OS-specific code
* Acts purely as orchestration glue

---



## 4. Data Stores

Torilate uses **no persistent data stores**.

All operations are:

* in-memory
* request-scoped
* stateless

---

## 5. External Integrations

### Tor Network

* Accessed via a local Tor SOCKS proxy
* Common ports:

  * `9050` — system Tor service
  * `9150` — Tor Browser bundle

Torilate does **not** interact with the Tor ControlPort.

---

## 6. Build & Deployment

**Build System**

* CMake (minimum version 3.20)

**Supported Compilers**

* GCC (Linux / MinGW)
<!-- * Clang -->

**Output**

* Single native executable
* No runtime dependencies beyond:

  * system C library
  * Tor SOCKS proxy

---

## 7. Security Considerations

* No local DNS resolution when using SOCKS4a
* Public-facing IP is determined by Tor exit node
* Plaintext HTTP offers no content confidentiality
* TLS / HTTPS support is not yet implemented

---

## 8. Development Notes

**Language Standard**

* C11

**Code Style**

* Explicit layering
* Minimal headers
* No cross-layer leakage
* Warnings enabled (`-Wall -Wextra -Wpedantic`)

**Testing**

* Manual verification via known IP-check endpoints
* No automated test suite yet

---

## 9. Future Roadmap

Planned architectural extensions include:

* TLS layer (HTTPS support)
* SOCKS5 support
* HTTP redirect handling
* Improved HTTP response parsing
* Optional Tor ControlPort integration
* Optional library mode (reusable core)

---

## 10. Project Identification

* **Project Name:** Torilate
* **Author:** Trident Apollo
* **Repository:** (add GitHub URL)
* **Last Updated:** 2026-01-25

---

## 11. Glossary

* **SOCKS4a** — SOCKS4 extension allowing proxy-side DNS resolution
* **Exit Node** — Final Tor relay that connects to the destination server
* **Tunnel** — A proxied TCP connection established via SOCKS

---


Good evening, Sir. Lenerd here.

You’re right to call this out — **`util` absolutely belongs in Core Components**, not as a footnote. It’s a *supporting infrastructure layer* that multiple layers depend on, and documenting it clarifies dependency direction and design intent.

Below is a **clean, architecture-consistent section** you can drop straight into **Section 3: Core Components**.

---
