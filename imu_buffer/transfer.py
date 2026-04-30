import argparse
import serial
import time
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Read stored IMU log from ESP32 over serial.")
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM6 on Windows or /dev/ttyUSB0 on Linux")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--out", default="imu_log.csv")
    parser.add_argument("--clear-after", action="store_true")
    args = parser.parse_args()

    output_path = Path(args.out)

    with serial.Serial(args.port, args.baud, timeout=2) as ser:
        time.sleep(2)

        ser.reset_input_buffer()

        print("Requesting log from ESP32...")
        ser.write(b"READ\n")

        in_log = False
        lines = []

        while True:
            raw = ser.readline()

            if not raw:
                break

            line = raw.decode("utf-8", errors="ignore").strip()

            if line == "BEGIN_LOG":
                in_log = True
                continue

            if line == "END_LOG":
                break

            if in_log:
                if line and line != "NO_LOG_FILE":
                    lines.append(line)

        if not lines:
            print("No log data received.")
            return

        output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

        print(f"Saved {len(lines) - 1} data rows to {output_path}")

        if args.clear_after:
            ser.write(b"CLEAR\n")
            print("Sent CLEAR command to ESP32")


if __name__ == "__main__":
    main()