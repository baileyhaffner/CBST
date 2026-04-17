import socket

HOST = "192.168.4.1"
PORT = 5000

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

buffer = ""

try:
    while True:
        data = sock.recv(1024).decode("utf-8", errors="ignore")
        if not data:
            break

        buffer += data

        while "\n" in buffer:
            line, buffer = buffer.split("\n", 1)
            line = line.strip()
            if line:
                print(line)
                print("\n")
                      for i in j:
                      \lambda while True,


except KeyboardInterrupt:
    pass
finally:
    sock.close()