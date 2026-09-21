# Forge

A minimal, from-scratch LLM inference server in C++17. Forge accepts HTTP inference
requests, schedules them through a priority queue with aging, and runs them across a
pool of worker threads backed by [`llama.cpp`](https://github.com/ggerganov/llama.cpp).

It is written deliberately without a web framework or job-queue library — the socket
handling, scheduling, and worker lifecycle are all implemented directly — as a study of
how an inference serving layer fits together.

## Architecture

```
HTTP client
    │  POST /infer
    ▼
Server ───── one detached thread per connection
    │        parses JSON, creates a Job, blocks on std::future
    ▼
Scheduler ── priority_queue<Job> + aging thread (raises priority every 1s)
    │
    ▼
Worker pool ─ N threads, each owning its own llama_context
             runs inference, fulfills the Job's promise, records latency
```

A request's lifetime:

1. `Server::handleClient` parses the request body and builds a `Job` carrying the query
   and a `std::promise<std::string>`.
2. The job is enqueued; the connection thread blocks on the matching `std::future`.
3. A worker wakes on the condition variable, dequeues the highest-priority job, and runs
   greedy-sampled generation against its own `llama_context`.
4. The worker sets the promise value, unblocking the connection thread, which writes the
   HTTP response and records the end-to-end latency.

## Requirements

- Linux (the networking layer uses POSIX sockets directly)
- A C++17 compiler and CMake ≥ 3.14
- [`llama.cpp`](https://github.com/ggerganov/llama.cpp), built as a shared library
- [`nlohmann/json`](https://github.com/nlohmann/json) — `pacman -S nlohmann-json`
  (Debian/Ubuntu: `apt install nlohmann-json3-dev`)
- A GGUF model file (developed against `Llama-3.2-1B-Instruct-Q4_K_M.gguf`)

## Configuration

All paths are resolved at configure time — nothing needs editing by hand.

| Option | Default | Purpose |
| --- | --- | --- |
| `LLAMA_CPP_DIR` | sibling `../llama.cpp`, else `LLAMA_CPP_DIR` from the environment | Where to find `llama.h` and `libllama`. Searches an in-tree build (`build/bin`), an installed prefix, and system paths. |
| `FORGE_MODEL_PATH` | unset | Bakes the GGUF path in as `MODEL_PATH`. If unset, the server exits at startup with a message. |
| `NLOHMANN_JSON_INCLUDE_DIR` | auto-detected | Only needed if `nlohmann/json` is installed somewhere non-standard. |

```bash
cmake -B build \
  -DLLAMA_CPP_DIR=/path/to/llama.cpp \
  -DFORGE_MODEL_PATH=/path/to/Llama-3.2-1B-Instruct-Q4_K_M.gguf
```

Configuration fails early with an explanatory message if `llama.cpp` or
`nlohmann/json` cannot be found.

Server defaults are still set at construction in `main.cpp`:

| Setting | Default | Defined in |
| --- | --- | --- |
| Port | `8080` | `main.cpp` |
| Worker threads | `4` | `Server` constructor |
| Max tokens generated | `1028` | `Worker::n_predict` |
| Aging interval | 1s | `Scheduler::aging_loop` |

## Build

```bash
cmake -B build
cmake --build build
```

With `llama.cpp` checked out next to this repo, that is the whole build.

## Run

```bash
./build/forge
# server listening on port: 8080
```

## API

| Method | Endpoint | Body | Response |
| --- | --- | --- | --- |
| `POST` | `/infer` | `{"query": "..."}` | `200` `{"response": "<generated text>"}`, or `400` `{"error":"bad request"}` on an unparseable body |
| `GET` | `/metrics` | — | `200` `{"p99_latency_ms": <float>}` |

```bash
curl -X POST http://localhost:8080/infer -d '{"query": "Hello, how are you?"}'
curl http://localhost:8080/metrics
```

### Load testing

`post.lua` is a [`wrk`](https://github.com/wg/wrk) script for driving the inference endpoint:

```bash
wrk -t4 -c16 -d30s -s post.lua http://localhost:8080/infer
```

## Project layout

| File | Responsibility |
| --- | --- |
| `main.cpp` | Loads the model, constructs the scheduler and server, starts the accept loop. |
| `server.hxx` | Socket setup, HTTP parsing, request/response handling, metrics endpoint. |
| `scheduler.hxx` | Priority queue, enqueue/dequeue, background aging thread. |
| `worker.hxx` | Worker thread loop, `llama.cpp` context lifecycle, tokenization and sampling. |
| `job.hxx` | The unit of work: query, priorities, timestamps, promise. |
| `post.lua` | `wrk` load-test script. |

## Design notes

- **Priority ordering.** `Scheduler::JobComparator` orders `unique_ptr<Job>` by
  `currPriority`; without an explicit comparator the queue cannot order smart pointers
  meaningfully and aging has no effect.
- **Avoiding recursive locking.** Workers wait on the condition variable using
  `std::unique_lock` and then call `deque_locked()`, which assumes the lock is already
  held. Calling the self-locking `deque()` at that point would deadlock.
- **Worker-to-connection channel.** Each `Job` carries a
  `shared_ptr<promise<std::string>>`. The connection thread holds the future while the
  worker fulfills the promise — this is what carries generated text back across threads.
- **One context per worker.** Each `Worker` creates its own `llama_context` from the
  shared `llama_model` and frees it in its destructor, so contexts are never shared
  between threads.
- **Per-request KV cache reset.** `runInference` calls `llama_memory_clear` before
  tokenizing. Because a worker reuses one context across jobs, skipping this would let
  the previous request's KV cache condition the next request's output.
- **Bounded latency buffer.** `Scheduler::latencies` is a `std::deque<float>` trimmed to
  `MAX_QUEUE_SIZE` (1024) samples, so metrics memory stays flat over a long-running
  process rather than growing per request.

## Status and known limitations

This is a learning project, not production software. Current rough edges:

- HTTP parsing is minimal: requests are matched by substring and assumed to arrive in a
  single 4 KB `recv`, so bodies larger than that are truncated.
- `bind` and `listen` return values are not checked.
- There is no request timeout, backpressure, or graceful shutdown path for the server
  accept loop.
- The p99 figure is computed over the most recent 1024 samples only, by nearest-rank on
  a sorted copy taken under the scheduler mutex.
