# Forge: Architectural Concepts & Data Flow

This document explains the "Big Ideas" behind Forge. It focuses on how data flows through the system and how threads interact to provide high-performance inference.

---

## 1. High-Level System Flow
The following diagram shows the lifecycle of a single request from the moment it hits the server until the response is sent.

```text
[ Client ]
    |
    | (1) TCP Connection
    v
[ Accept Loop ] --(2) Spawns--> [ HTTP Handler Thread ]
                                      |
                                      | (3) Parses JSON & Creates Job
                                      v
                                [ Scheduler (Priority Queue) ]
                                      |
       (5) Fulfills Promise <---------+---------- (4) Picks Highest Priority Job
               |                      |                    |
               v                      |                    v
[ HTTP Handler Thread ]               |           [ Worker Thread Pool ]
               |                      |                    |
               | (6) Sends Response   |                    | (Inference via llama.cpp)
               v                      |                    |
[ Client ] <--------------------------+--------------------'
```

---

## 2. The Threading Model (The "Restaurant" Analogy)

Forge uses three distinct types of threads to ensure that no single slow inference job blocks the entire server.

### A. The Accept Loop (The Receptionist)
*   **Count**: Exactly 1.
*   **Role**: Listens at the door (Port 8080). Its only job is to say "Hello" to a new connection and immediately hand it off to a Waiter. It never does "work" so it's always ready for the next guest.

### B. HTTP Handler Threads (The Waiters)
*   **Count**: Dynamic (one per request).
*   **Role**: They take the order (parse the JSON), put it in the kitchen (The Scheduler), and then **stand by the window** waiting for the food. Because they are "detached," the server can have hundreds of people "waiting at their tables" without stopping the kitchen from working.

### C. Worker Threads (The Chefs)
*   **Count**: Fixed (Default: 4).
*   **Role**: These are the "Heavy Lifters." They don't know about the internet or JSON; they only know how to take a Job from the Scheduler and run the Transformer model. Each Chef has their own "Stove" (`llama_context`) so they don't trip over each other.

---

## 3. Priority Scheduling & Aging

In a simple server, requests are handled First-In-First-Out (FIFO). In Forge, we use a **Priority Queue**.

### The Problem: Starvation
If high-priority "V.I.P" requests keep coming in, a low-priority "Regular" request might sit in the queue forever. This is called **Starvation**.

### The Solution: Aging
Forge runs a background **Aging Thread**. Every second, it sweeps through the queue and "ages" the jobs:
1.  It pulls all jobs out.
2.  It increases their `currPriority` by a small amount (e.g., `+0.2`).
3.  It puts them back in.

**Result**: Over time, even the lowest priority job will eventually become a "V.I.P" and get processed by a worker.

---

## 4. The Bridge: Promise & Future

This is the most critical concept for connecting the "Internet" (Asynchronous) to the "Model" (Synchronous).

1.  **The Promise**: When a Waiter (Handler) creates a job, they create a **Promise**. This is a "contract" that says "I will eventually give you a string."
2.  **The Future**: The Waiter holds the **Future** (the other end of the contract). They call `.get()`, which puts the thread to sleep until the contract is fulfilled.
3.  **The Fulfillment**: When a Chef (Worker) finishes the math, they call `set_value()` on the Promise. This "wakes up" the specific Waiter waiting for that specific job.

---

## 5. Memory & Resource Isolation

*   **Model**: Loaded once and shared by everyone (read-only).
*   **Context**: Each Worker has its own `llama_context`. This is essential because the context stores the "memory" of the current inference. If workers shared a context, their tokens would get mixed up like a scrambled puzzle.
*   **Locks**: We use `std::mutex` to ensure that when two Chefs reach for a job at the same time, only one gets it.
