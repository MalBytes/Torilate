
# Torilate

> [!WARNING]
> Torilate is currently in an **alpha** stage of development.  
> The software is experimental, under active development, and may undergo significant changes.  
> Stability, security guarantees, and backward compatibility are **not yet assured**.

**Torilate** is a lightweight command-line tool that routes network requests through the **Tor network**, allowing users to access remote services via a Tor exit node without modifying the underlying application.

Torilate works by establishing a Tor-backed SOCKS tunnel and issuing requests through it, making it suitable for privacy-aware networking, testing, and research use cases.

---

## Key Features

* Route HTTP requests through the Tor network
* SOCKS4 / SOCKS4a compliant (proxy-side DNS resolution supported)
* No local DNS leaks when using hostname mode
* Cross-platform (Windows and Linux)
* Minimal dependencies, native C implementation
* Designed for transparency and correctness

---

## How It Works (High Level)

```
Torilate → Tor SOCKS Proxy → Tor Exit Node → Target Server
```

Torilate connects to a locally running Tor SOCKS proxy (e.g. `127.0.0.1:9050`), establishes a tunnel, and sends requests through that tunnel.
The destination server sees the **Tor exit node’s IP**, not yours.

---

> [!NOTE]
> No official releases will be published until Torilate reaches the **beta** stage.  
> Development builds may change rapidly and are intended for testing and evaluation purposes only.

## Requirements

* A running Tor service at `127.0.0.1:9050`
    * [Download Expert TOR Bundle](https://www.torproject.org/download/tor/)

* CMake ≥ 3.20
  
* A C compiler (GCC or Clang)

---

## Building

```bash
cmake -S . -B build
cmake --build build
```

The resulting binary will be placed in:

```
./bin/torilate
```

---

## Usage

### Basic HTTP GET request through Tor

```bash
torilate <command> <url> [options] [flags]
```

### Command Examples

```bash
torilate get <url> [-r|--raw]
```
```bash
torilate post <url> [-t|--content-type=<content-type>] [-b|--body=<body>] [-r|--raw]
```

### Verify Tor routing

```bash
torilate get http://httpbin.org/ip
```

Expected response:

```json
{
  "origin": "<tor-exit-ip>"
}
```

If the IP differs from your real public IP, the request is successfully routed through Tor.

---

## Security Notes

* Torilate does **not** perform local DNS resolution when using hostname mode
* Public IP visibility depends on Tor exit nodes
* HTTPS support requires a TLS layer (planned, not yet implemented)
* Torilate does not interact with Tor’s ControlPort

---

## Project Status

Torilate is under **active development**.

Planned improvements include:

- [x] URI parsing & resolution
- [x] HTTP redirect handling
- [x] Improved protocol parsing
- [ ] HTTPS (TLS) support
- [ ] HTTP chunked transfer encoding handling
- [ ] SOCKS5 support
- [ ] Optional Tor circuit control

See [`ARCHITECTURE.md`](https://github.com/MalBytes/Torilate/blob/main/ARCHITECTURE.md) for design details.

---

## Contributing

Contributions are welcome.

Please see:

* [`ARCHITECTURE.md`](https://github.com/MalBytes/Torilate/blob/main/ARCHITECTURE.md)
* [`CONTRIBUTING.md`](https://github.com/MalBytes/Torilate/blob/main/CONTRIBUTING.md)
* [`CODE_OF_CONDUCT.md`](https://github.com/MalBytes/Torilate/blob/main/CODE_OF_CONDUCT.md)
* [`COMMIT_GUIDELINES.md`](https://github.com/MalBytes/Torilate/blob/main/COMMIT_GUIDELINES.md)

All changes are accepted via Pull Requests.





