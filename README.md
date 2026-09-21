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

Two paths must be set before the project will build and run:

| What | Where | Notes |
| --- | --- | --- |
| Model path | `MODEL_PATH` in `main.cpp` | Empty by default; the server exits with an error if unset. |
| `llama.cpp` location | `target_include_directories` / `target_link_libraries` in `CMakeLists.txt` | Currently absolute paths; point them at your own checkout. |

Server defaults are set at construction in `main.cpp`:

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

## Run

```bash
./build/forge
# server listening on port: 8080
```

## API

| Method | Endpoint | Body | Response |
| --- | --- | --- | --- |
| `POST` | `/infer` | `{"query": "..."}` | Generated text |
| `GET` | `/metrics` | — | `{"p99_latency_ms": <float>}` |

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

## Status and known limitations

This is a learning project, not production software. Current rough edges:

- **The tree does not compile as-is.** `server.hxx` and `worker.hxx` reference
  `sch.latencies`, but `Scheduler` declares no such member; it needs a
  `std::vector<float> latencies` guarded by the existing mutex.
- **Aging is a no-op.** `aging_rate` is a `float` (`0.2`) added to an `int`
  `currPriority`, so the increment truncates to zero. Either make the priority
  fractional or use an integer step.
- Build artifacts (`build/`) and `server.log` are currently tracked in git.
- HTTP parsing is minimal: requests are matched by substring, assumed to arrive in a
  single 4 KB `recv`, and malformed JSON is not handled.
- `bind` and `listen` return values are not checked.
- There is no request timeout, backpressure, or graceful shutdown path for the server
  accept loop.
