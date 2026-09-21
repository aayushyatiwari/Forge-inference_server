# ForgeCopy

A manual rewrite of the Forge inference server, focusing on manual implementation of error handling, scheduling, and llama.cpp integration.

## Features
- **Priority Scheduler**: Implements a priority queue for jobs with a background aging thread to prevent starvation.
- **Worker Pool**: Multi-threaded worker system where each worker manages its own `llama_context`.
- **HTTP Server**: Simple socket-based server handling inference requests and performance metrics.
- **Metrics**: Track P99 latency for inference jobs.

## Recent Logical & Architectural Fixes
- **Priority Queue Implementation**: Added a custom `JobComparator` to `Scheduler`. Without this, the priority queue could not compare `unique_ptr<Job>` objects, and the aging system would not have functioned.
- **Deadlock Prevention (Locked Dequeue)**: Introduced `deque_locked()` and switched `Worker` threads to `std::unique_lock`. This prevents the recursive locking deadlock that occurs when a worker wakes from a condition variable and immediately calls a locking dequeue function.
- **Communication Channel Correction**: Updated the `Job` promise/future pair from `std::mutex` to `std::string`. This was necessary to actually transmit the generated text from the worker back to the server's response handler.
- **Llama.cpp Lifecycle Management**: Fixed the `Worker` initialization to correctly sequence `llama_init_from_model` and added proper `llama_free` cleanup in destructors to prevent memory leaks during context switching.
- **Tokenization & Inference Logic**: Refactored `runInference` to correctly handle the two-pass tokenization pattern (size check then allocation) and fixed the inference loop bounds to account for both prompt tokens and the prediction limit.
- **Socket API Correctness**: Fixed the networking stack to properly initialize `sockaddr_in` structures and ensure client connections are closed in all code paths (error and success) to prevent file descriptor leaks.
- **Build System Integrity**: Fixed the `CMakeLists.txt` and `main.cpp` entry points to ensure the model path and library linking were valid for the local environment.

## Prerequisites
- `llama.cpp` installed/built in `~/code/llama.cpp`.
- `nlohmann/json` library installed (`libnlohmann-json-dev`).
- A GGUF model file (default path: `/home/imaayush/code/Forge/Forge/Llama-3.2-1B-Instruct-Q4_K_M.gguf`).

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

**Inference Request:**
```bash
curl -X POST http://localhost:8080/infer -d '{"query": "Hello, how are you?"}'
```

**Metrics Request:**
```bash
curl http://localhost:8080/metrics
```

## Implementation Details
- **Scheduler**: Uses a `std::priority_queue` with a custom comparator. An aging thread increases job priority every second.
- **Workers**: Each worker waits on a condition variable, pulls a job from the scheduler, and runs inference using the `llama.cpp` C++ API.
- **Networking**: Uses raw Linux sockets to handle HTTP/1.1 requests.
