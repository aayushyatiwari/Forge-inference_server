# Queue

A robust priority-based job scheduling system implemented in Python. This component focuses on the logic of managing task execution orders and ensuring fairness in a multi-worker environment.

## Features

- **Priority Levels**: Support for "Urgent" (Priority 2) and "Batch" (Priority 1) jobs.
- **Aging Mechanism**: A background thread monitors the queue and "ages" jobs. If a batch job waits too long, its priority is boosted to prevent it from being starved by a constant stream of urgent jobs.
- **Thread-Safe Scheduler**: Uses a `heapq` based priority queue protected by `threading.Lock` and `threading.Condition`.
- **Worker Pool**: Multiple worker threads simulate concurrent processing of jobs.

## Components

- `job.py`: Data structure for tasks.
- `scheduler.py`: The core scheduling logic and aging thread.
- `worker.py`: Worker logic that consumes jobs.
- `main.py`: Simulation entry point.

## Logic: Aging vs Starvation

Without aging, a flood of Priority 2 jobs would mean Priority 1 jobs never run. The aging thread ensures that every job eventually gets processed by boosting the priority of older jobs after a configurable timeout (e.g., 30 seconds).

## Usage

Run the simulation and check `forge.log` for execution order:
```bash
python main.py
```
