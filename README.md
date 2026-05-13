# Forge: High-Performance Inference Server Workspace

This workspace contains a series of server implementations built from scratch, evolving from a simple toy HTTP server to a sophisticated C++ inference server with job scheduling and priority management.

## Project Structure

The workspace is divided into three main components:

### 1. [Bongo](./bongo) - Toy HTTP Server Implementations
A collection of Python-based HTTP servers built using raw sockets. It explores different concurrency models:
- **Dumb Server**: Single-threaded, blocking server for learning the basics of HTTP over TCP.
- **Threaded Server**: Multi-threaded approach using Python's `threading` module.
- **Async Server**: High-concurrency implementation using `asyncio`.

**Key Benchmarks (POST /reverse):**
| Implementation | Latency (Avg) | Req/Sec |
| :--- | :--- | :--- |
| Thread-based | 60.09 ms | 1,649.56 |
| Asyncio-based | 15.87 ms | 11,326.42 |

### 2. [Queue](./queue) - Priority Job Scheduler
A standalone Python implementation of a priority-based job scheduling system. It features:
- **Priority Queueing**: Urgent jobs vs. Batch jobs.
- **Aging Mechanism**: A dedicated thread that prevents starvation by gradually increasing the priority of long-waiting jobs.
- **Thread-Safety**: Robust locking for concurrent job submission and worker processing.

### 3. [Forge](./Forge) - C++ LLM Inference Server
The capstone project combining high-performance C++ networking with LLM inference.
- **Architecture**: Separates network handling (detached threads) from compute (worker thread pool) using a scheduler and promise/future bridge.
- **LLM Integration**: Uses `llama.cpp` to serve Llama-3.2-1B-Instruct.
- **Metrics**: Integrated tracking for queue depth and p99 latency.

**Inference Benchmarks (Llama-3.2-1B):**
- **Avg Latency**: 8.81s
- **P99 Latency**: 12.29s
- **Throughput**: ~0.20 Req/Sec (Model-bound)

## Benchmarking Tools
We use `wrk` for load testing across all projects. Custom Lua scripts (e.g., `post.lua`) are used to benchmark POST endpoints with JSON payloads.

---
*Created as part of the Forge systems programming series.*
