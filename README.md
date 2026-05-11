# Forge — An Inference Server Built From Scratch

This repo is a 4-project arc in systems engineering. Each project builds on the last, starting from raw TCP sockets in Python and ending with a working C++ inference server backed by llama.cpp.

No frameworks. Every abstraction earned.

---

## The Arc

### Project 1 — Bongo (Python)
A raw HTTP server using only sockets. No frameworks. Accepts `POST` with JSON, returns JSON.

Learned: HTTP is text over TCP. `recv()` gives raw bytes. Headers and body split on `\r\n\r\n`.

### Project 2 — Concurrent Bongo (Python)
Threaded and async versions of Bongo. Benchmarked with `wrk` at 1000 connections. Async won on throughput and latency.

Learned: thread-per-connection vs event loop tradeoffs. Where blocking I/O hurts.

### Project 3 — Priority Job Scheduler (Python)
A priority queue job scheduler. Jobs tagged `urgent` (priority=2) or `batch` (priority=1). Workers pull jobs in priority order.

Intentionally induced starvation — flooded with urgent jobs, watched batch jobs wait 5-7 seconds. Fixed with aging — a background thread boosts `currPriority` over time.

Learned: `heapq`, `threading.Lock`, `threading.Thread`. Observed starvation and aging in logs.

### Project 4 — Forge (C++)
A full inference server in C++. Every piece from Projects 1-3 rebuilt and wired to a real LLM via llama.cpp.

---

## Forge Architecture

```
HTTP Request
    │
    ▼
Server (raw socket, accept loop)
    │
    ▼
handleClient() — parses HTTP, creates Job
    │
    ├── Job { id, query, st, et, oldPriority, currPriority, promise }
    │
    ▼
Scheduler (thread-safe priority queue)
    ├── std::priority_queue<unique_ptr<Job>>
    ├── std::condition_variable — workers sleep until job arrives
    └── aging thread — boosts currPriority every second
    │
    ▼
Worker threads (4 by default)
    ├── wait on condition variable
    ├── dequeue top job
    ├── runInference(query) → llama.cpp
    └── promise.set_value(result) — unblocks HTTP handler
    │
    ▼
HTTP Response
```

**Promise/Future per request:** the HTTP handler creates a `std::promise`, passes it with the Job, then blocks on `future.get()`. The worker fulfills the promise after inference. Zero polling.

---

## File Structure

```
Forge/
├── main.cpp          — model loading, wires Scheduler + Server
├── job.hxx           — Job struct
├── scheduler.hxx     — priority queue, aging, condition variable
├── worker.hxx        — worker threads, llama.cpp inference
├── http.hxx          — raw socket HTTP server, /infer, /metrics
└── CMakeLists.txt
```

---

## Building

**Dependencies:**
- llama.cpp (built from source)
- nlohmann/json (header-only)
- A GGUF model file

**Build llama.cpp:**
```bash
git clone https://github.com/ggml-org/llama.cpp
cd llama.cpp
cmake -B build
cmake --build build --config Release -j4
```

**Download a model:**
```bash
wget https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF/resolve/main/Llama-3.2-1B-Instruct-Q4_K_M.gguf
```

**Update paths in CMakeLists.txt** to point to your llama.cpp install and `libllama.so`.

**Update model path in main.cpp.**

**Build Forge:**
```bash
cd Forge
mkdir build && cd build
cmake ..
make
./forge
```

---

## Usage

**Inference:**
```bash
curl -X POST http://localhost:8080/infer \
  -H "Content-Type: application/json" \
  -d '{"query": "what is 2+2?"}'
```

**Metrics:**
```bash
curl http://localhost:8080/metrics
# {"queue_depth": 0, "p99_latency_ms": 1047.0}
```

---

## C++ Primitives Used

- `std::unique_ptr` + `std::move` — ownership transfer, no manual memory management
- `std::shared_ptr` — shared ownership for promise across Job and HTTP handler
- `std::mutex` + `std::lock_guard` — RAII mutex locking
- `std::condition_variable` — workers sleep until job arrives, zero CPU spin
- `std::atomic<bool>` — thread-safe shutdown flag
- `std::thread` — worker threads and aging loop
- `std::promise` / `std::future` — connect HTTP handler to worker result
- `std::priority_queue` with custom comparator — priority scheduling