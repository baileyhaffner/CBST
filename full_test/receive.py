import socket

HOST = "192.168.4.1"
PORT = 3333

filename = "imu_data.csv"

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    print("Connected to ESP32")

    recording = False

    with open(filename, "w") as file:
        while True:
            data = s.recv(1024).decode(errors="ignore")

            if not data:
                break

            for line in data.splitlines():

                if line == "START_CSV":
                    recording = True
                    print("Receiving CSV...")
                    continue

                if line == "END_CSV":
                    print("Saved CSV!")
                    recording = False
                    break

                if recording:
                    file.write(line + "\n")
                    print(line)