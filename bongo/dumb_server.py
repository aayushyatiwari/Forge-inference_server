# build the dumbest possible server, for learning purposes
import json
import socket


HOST = 'localhost'  # Standard loopback interface address (localhost)
PORT = 8080        # Port to listen on (non-privileged ports are > 1023)
endpoint = "/reverse"
DEBUG = False


def build_response(status, content_type, body):
    body_bytes = body.encode()
    headers = (
        f"HTTP/1.1 {status}\r\n"
        f"Content-Type: {content_type}\r\n"
        f"Content-Length: {len(body_bytes)}\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    return headers.encode() + body_bytes


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT)) # bind the socket to host and port
    s.listen() # listeno for connections
    print(f"server listening on {HOST}:{PORT}")
    while True:
        conn, addr = s.accept() # accept a connection
        with conn:
            if DEBUG:
                print(f"CONECTION FROM {addr}")
            data = conn.recv(1024) # receive data from the client 1024 bytes at a time
            request = data.decode()
            request_line = request.split('\r\n')[0] # get the first line of the request
            if DEBUG:
                print(f"RECEIVED REQUEST: {request_line}")
            if request_line.startswith(f"POST {endpoint}"):
                body = request.split('\r\n\r\n', 1)[1]
                json_data = json.loads(body)
                rd = [x[::-1] for x in list(json_data.values())]
                rev_json = json.dumps({"data": rd})
                response = build_response("200 OK", "application/json", rev_json)
                if DEBUG:
                    print(f"SENT RESPONSE: {rev_json}")
            else:
                response = build_response("404 Not Found", "text/plain", "Endpoint not found")
                if DEBUG:
                    print("SENT RESPONSE: Endpoint not found")
            conn.sendall(response) # send the response to the client
    s.close()


'''
make a server that listens on a port, accepts a connection and then sends a response and closes the connection.
The response should be a simple HTTP response with a status code of 200 and a body of "Hello, World!".
NOTES:
- socket.socket() creates a new socket object using the given address family and socket type.
- socket.AF_INET is the address family for IPv4.
- socket.SOCK_STREAM is the socket type for TCP.
- s.bind() binds the socket to the specified host and port.
- s.listen() enables the server to accept connections. The argument specifies the maximum number of queued connections.
- s.accept() waits for an incoming connection and returns a new socket object representing the connection
- and the address of the client
- conn.recv() receives data from the client. The argument specifies the maximum amount of data to be received at once.
- conn.sendall() sends data to the client. It ensures that all data is sent before returning.
- the server deals in bytes. we need to encode strings to bytes before sending and also decode them when we recive them.
'''
