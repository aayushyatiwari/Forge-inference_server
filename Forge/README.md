# Forge

Forge is a high-performance LLM inference server written in C++. It bridges the gap between raw networking and heavy-duty machine learning compute by providing a multi-threaded, priority-aware scheduling system for llama.cpp.

## Architecture

Forge uses a decoupled architecture to maximize throughput:
1. **Accept Loop**: Listens for TCP connections on a dedicated thread.
2. **HTTP Handlers**: Each connection is handled in a detached thread that parses the request and enqueues a `Job`.
3. **Promise/Future Bridge**: The handler thread blocks on a `std::future`, waiting for the worker to fulfill the promise with the inference result.
4. **Priority Scheduler**: A thread-safe priority queue manages jobs. An **Aging Thread** runs in the background to increment the priority of waiting jobs, preventing starvation.
5. **Worker Pool**: A pool of worker threads consumes jobs. Each worker maintains its own `llama_context` to allow concurrent inference operations.

## Model
The server is optimized for **Llama-3.2-1B-Instruct** in Q4_K_M GGUF format.

## Benchmarks

Benchmarked with `wrk` under model load (simulating heavy inference):

| Metric | Value |
| :--- | :--- |
| **Avg Latency** | 7.30 s |
| **P50 Latency** | 7.27 s |
| **P99 Latency** | 7.62 s |
| **Requests/sec** | 0.40 |

## How to Run

### 1. Build
```bash
mkdir -p build && cd build
cmake ..
make
```

### 2. Start Server
```bash
./forge
```

### 3. Usage
```bash
curl -X POST http://localhost:8080/infer -d '{"query": "Explain the concept of priority aging."}'
```

## Metrics
The server exposes a `/metrics` endpoint:
- `p99_latency_ms`: 99th percentile latency.
- `queue_depth`: Number of jobs currently waiting in the scheduler.
