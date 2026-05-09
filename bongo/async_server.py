import asyncio
import json

HOST = 'localhost'
PORT = 8080
endpoint = "/reverse"

async def handle_connection(reader, writer):
    try:
        data = await reader.read(1024)
        request_line = data.decode().split('\r\n')[0]
        if request_line.startswith(f"POST {endpoint}"):
            body = data.decode().split('\r\n\r\n')[1]
            json_data = json.loads(body)
            rd = [x[::-1] for x in list(json_data.values())]
            rev_json = json.dumps({"data": rd})
            response = f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{rev_json}"
        else:
            response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nEndpoint not found"
        writer.write(response.encode())
        await writer.drain()
    except Exception as e:
        print(f"ERROR: {e}")
    finally:
        writer.close()
        await writer.wait_closed()

async def main():
    server = await asyncio.start_server(handle_connection, HOST, PORT)
    print(f"server listening on {HOST}:{PORT}")
    async with server:
        await server.serve_forever()

asyncio.run(main())