# Forge (C++ Inference Server)

Forge is a high-performance LLM inference server written in C++. it bridges the gap between raw networking and heavy-duty machine learning compute.

## Architecture

Forge uses a decoupled architecture to maximize throughput:
1. **Accept Loop**: Listens for TCP connections.
2. **HTTP Threads**: Each connection is handled in a detached thread that parses the request and enqueues a `Job`.
3. **Promise/Future Bridge**: The HTTP thread blocks on a `std::future`, waiting for the worker to finish.
4. **Worker Pool**: A pool of worker threads consumes jobs from the `Scheduler`.
5. **Inference Engine**: Workers run inference using `llama.cpp` and fulfill the `std::promise`, waking up the HTTP thread to send the response.

## Model
Currently configured to serve **Llama-3.2-1B-Instruct** in Q4_K_M GGUF format.

## Benchmarks

Benchmarked with `wrk` under model load (simulating 3s inference per request):

| Metric | Value |
| :--- | :--- |
| **Avg Latency** | 8.81 s |
| **P50 Latency** | 7.33 s |
| **P99 Latency** | 12.29 s |
| **Requests/sec** | 0.20 |
| **Threads** | 2 |
| **Connections** | 4 |

## Build and Run

### Prerequisites
- `llama.cpp` library installed/built at `~/code/llama.cpp`.
- GGUF model file.

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### Run
```bash
./forge
```

## Metrics
The server exposes a `/metrics` endpoint that provides observability:
- `p99_latency_ms`: The 99th percentile latency of inference requests.
