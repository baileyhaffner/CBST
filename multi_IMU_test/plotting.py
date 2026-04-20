import serial
import csv
import time

# ==== CONFIG ====
PORT = 'COM6'          # change if needed
BAUD = 115200
OUTPUT_FILE = 'imu_dual_log.csv'

# ==== OPEN SERIAL ====
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)  # allow board/serial to settle

# ==== OPEN CSV ====
file = open(OUTPUT_FILE, mode='w', newline='')
writer = csv.writer(file)

writer.writerow([
    "time",
    "ax_A", "ay_A", "az_A", "gx_A", "gy_A", "gz_A", "temp_A",
    "ax_B", "ay_B", "az_B", "gx_B", "gy_B", "gz_B", "temp_B"
])

print(f"Logging from {PORT} to {OUTPUT_FILE}")
print("Press Ctrl+C to stop.\n")

imuA_data = None
imuB_data = None

def parse_line(line):
    parts = [p.strip() for p in line.split(",")]

    if len(parts) != 8:
        return None, f"Skipped: expected 8 fields, got {len(parts)}"

    imu_id = parts[0]

    if imu_id not in ("IMU_0x6A", "IMU_0x6B"):
        return None, f"Skipped: unknown IMU label '{imu_id}'"

    try:
        values = [float(x) for x in parts[1:]]
    except ValueError:
        return None, "Skipped: numeric conversion failed"

    return (imu_id, values), None

try:
    while True:
        raw = ser.readline()

        if not raw:
            continue

        try:
            line = raw.decode("utf-8", errors="replace").strip()
        except Exception as e:
            print("Decode error:", e)
            continue

        if not line:
            continue

        print("RAW:", line)

        # Skip obvious non-data lines
        if line.startswith("imu,"):
            print("Detected CSV header")
            continue

        result, error = parse_line(line)

        if error:
            print(error)
            continue

        imu_id, values = result

        if imu_id == "IMU_0x6A":
            imuA_data = values
            print("Stored IMU 0x6A sample")

        elif imu_id == "IMU_0x6B":
            imuB_data = values
            print("Stored IMU 0x6B sample")

        if imuA_data is not None and imuB_data is not None:
            timestamp = time.time()
            row = [timestamp] + imuA_data + imuB_data
            writer.writerow(row)
            file.flush()

            print("WROTE CSV ROW")

            imuA_data = None
            imuB_data = None

except KeyboardInterrupt:
    print("\nLogging stopped by user.")

finally:
    file.close()
    ser.close()
    print("Serial port and file closed.")