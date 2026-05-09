# Queue Component

A priority-based job scheduling system with an aging mechanism to prevent starvation.

## Components

- **`job.py`**: Defines the `Job` class, which encapsulates task data, priority (2 for urgent, 1 for batch), and timing metadata.
- **`scheduler.py`**: Implements a thread-safe `Scheduler` using a priority queue (heap). Includes an aging thread that gradually increases the priority of waiting jobs to ensure eventual execution.
- **`worker.py`**: Contains the worker logic that retrieves and processes jobs from the scheduler.
- **`main.py`**: The entry point that initializes the scheduler, starts multiple worker threads, and simulates an incoming stream of jobs.

## Features

- **Priority Scheduling**: Urgent jobs are prioritized over batch jobs.
- **Starvation Prevention**: The aging mechanism ensures that low-priority jobs don't wait indefinitely.
- **Thread-Safe**: Uses locks to manage concurrent access to the job queue.
- **Detailed Logging**: Tracks job lifecycle from creation to completion, including wait and processing times.

## Usage

To run the simulation:

```bash
python main.py
```

Logs will be written to `forge.log` and printed to the console.
