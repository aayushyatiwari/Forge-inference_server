# build a server that listens on a port, accepts a json, and then sends reverse of the json and closes the connection.
import socket
import json

'''
make a server that listens on a port, accepts a connection and then sends a response and closes the connection.
The response should be a simple HTTP response with a status code of 200 and a body of the reverse of the json sent by the client.
/reverse endpoint should reverse the json sent by the client and return it in the response body.
'''

HOST = 'localhost'  # Standard loopback interface address (localhost)
PORT = 8080        # Port to listen on (non-privileged ports are > 1023)
endpoint = "/reverse" # endpoint to reverse the json sent by the client

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT)) # bind the socket to host and port
    s.listen() # listeno for connections
    print(f"server listening on {HOST}:{PORT}")
    while True:
        conn, addr = s.accept() # accept a connection
        with conn:
            print(f"CONECTION FROM {addr}")
            data = conn.recv(1024) # receive data from the client 1024 bytes at a time, what if the client sends more than 1024 bytes?
            request_line = data.decode().split('\r\n')[0] # get the first line of the request
            print(f"RECEIVED REQUEST: {request_line}")

            # request line will be in the format "METHOD /endpoint HTTP/1.1"
            if request_line.startswith(f"POST {endpoint}"):
                body = data.decode().split('\r\n\r\n')[1] # get the body of the request
                json_data = json.loads(body) # parse the json data
                rd = list(json_data.values()) # get the value of the key data 
                rd = [x[::-1] for x in rd] # reverse the value of the key data
                rev_json = json.dumps({"data": rd}) # convert the reversed data back to json
                response = f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{rev_json}\r\n" # create a simple HTTP response
                conn.sendall(response.encode()) # send the response to the client
                print(f"SENT RESPONSE: {rev_json}")
            else:
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nEndpoint not found" # create a simple HTTP response for invalid endpoint
                conn.sendall(response.encode()) # send the response to the client
                print("SENT RESPONSE: Endpoint not found")