# Bongo

Bongo is a toy HTTP server built from scratch using raw sockets to understand the fundamentals of network programming and concurrency models in Python.

## Implementations

### 1. `dumb_server.py`
The most basic implementation. It handles one request at a time, blocking all other incoming connections until the current one is finished. Great for understanding the "line-at-the-bank" problem in networking.

### 2. `thread_server.py`
Uses Python's `threading` module to spawn a new thread for every incoming connection. This allows the server to handle multiple requests in parallel, but comes with the overhead of thread management.

### 3. `async_server.py`
A high-performance implementation using `asyncio`. It uses a single-threaded event loop to manage thousands of concurrent connections efficiently without the memory overhead of OS threads.

## Benchmarks

Benchmarked using `wrk -t4 -c1000 -d10s -s post.lua http://localhost:8080/reverse`.

| Metric | Thread-based Server | Asyncio-based Server |
| :--- | :--- | :--- |
| **Avg Latency** | 60.09 ms | 15.87 ms |
| **Max Latency** | 1.91 s | 831.44 ms |
| **Requests/sec** | 1,649.56 | 11,326.42 |
| **Transfer/sec** | 112.77 KB | 774.28 KB |

## Usage

1. Start a server: `python async_server.py`
2. Test with curl:
   ```bash
   curl -X POST http://localhost:8080/reverse -d '{"text": "hello"}'
   ```
3. Benchmark:
   ```bash
   wrk -t4 -c100 -d10s -s post.lua http://localhost:8080/reverse
   ```
