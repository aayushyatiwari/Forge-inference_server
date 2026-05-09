# Bongo

A collection of simple HTTP server implementations and testing scripts.

## Contents

- `async_server.py`: Asynchronous server using `asyncio`.
- `thread_server.py`: Multi-threaded server using `threading`.
- `dumb_server.py`: Basic server implementation.
- `server_takes_requests.py`: Alternative server implementation.
- `request.sh`: Shell script for testing servers with `curl`.
- `post.lua`: Lua script for request benchmarking.

## Usage

Servers typically listen on `localhost:8080` and expose a `POST /reverse` endpoint.
