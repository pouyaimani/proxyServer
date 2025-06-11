# Lightweight HTTP Proxy Server with Logging (C)

A lightweight multi-process HTTP proxy server written in C, designed for basic request forwarding and logging. The proxy logs incoming client requests, supports process forking per request, and returns logs via a hidden service endpoint.

---

## Features

- Forwards HTTP requests to an outbound server
- Logs each request with:
- Client IP
- Timestamp
- Forked Process PID
- Exposes logs via special endpoint: `/.svc/collect_logs`
- Uses `fork()` for concurrent request handling
- Sets CPU affinity dynamically for each forked child (if supported)
- Writes logs to file (`proxy_requests.log`) and memory

---

## Build Instructions

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Requires GNU/Linux environment and GCC.
```

Requires GNU/Linux environment.

---

## Run Example

```bash
cd build
./proxy_server --inbound 127.0.0.1:8080 --outbound 93.184.216.34
```

- `--inbound`: IP and port to bind the proxy server
- `--outbound`: IP address of the destination server (e.g. example.com = 93.184.216.34)

---

## API Behavior

| Endpoint                | Description              |
|------------------------|--------------------------|
| `/any_path`            | Forwarded to outbound    |
| `/.svc/collect_logs`   | Returns formatted logs   |

Logs look like:

```
IP: 127.0.0.1, Timestamp: Sat Jun 11 13:45:20 2025, PID: 12345
```

---

## Technical Details

- **Concurrency:** via `fork()` and `SIGCHLD` handler for cleanup  
- **CPU Affinity:** uses `sched_setaffinity()` on supported systems to distribute forked children across CPU cores  
- **Robustness:** `SA_RESTART` used to make interrupted system calls auto-resume  
- **Logging:** in-memory + file logging for persistent + quick access

---

## Project Structure

```
proxy_server_modular/
├── CMakeLists.txt
├── include/
│   ├── logger.h
│   ├── request_handler.h
│   └── signal_handler.h
├── src/
│   ├── logger.c
│   ├── main.c
│   ├── request_handler.c
│   └── signal_handler.c
```

---

## To Do / Ideas

- Add HTTPS support (via OpenSSL)
- Add max connection limit or pool
- Replace fork with pthreads or event loop for efficiency
- Add request filtering or caching

---

## License

MIT License
