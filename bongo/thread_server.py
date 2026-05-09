import socket
import json
import threading

HOST = 'localhost'
PORT = 8080
endpoint = "/reverse"

def handle_connection(conn, addr):
    with conn:
        try:
            data = conn.recv(1024)
            request_line = data.decode().split('\r\n')[0]
            if request_line.startswith(f"POST {endpoint}"):
                body = data.decode().split('\r\n\r\n')[1]
                json_data = json.loads(body)
                rd = [x[::-1] for x in list(json_data.values())]
                rev_json = json.dumps({"data": rd})
                response = f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{rev_json}"
                conn.sendall(response.encode())
            else:
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nEndpoint not found"
                conn.sendall(response.encode())
            print(f"CONNECTION FROM {addr} HANDLED SUCCESSFULLY")
        except Exception as e:
            print(f"ERROR: {e}")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # to close the socket immediately after the program ends
    s.bind((HOST, PORT))
    s.listen()
    print(f"server listening on {HOST}:{PORT}")
    while True:
        conn, addr = s.accept()
        thread = threading.Thread(target=handle_connection, args=(conn, addr))
        thread.start()