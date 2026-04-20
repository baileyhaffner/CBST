import serial
import matplotlib.pyplot as plt
from collections import deque

# ==== CONFIG ====
PORT = 'COM6'       # change if needed
BAUD = 115200
MAX_POINTS = 200    # how many samples to keep on screen

# ==== SERIAL ====
ser = serial.Serial(PORT, BAUD, timeout=1)

# ==== DATA STORAGE ====
imuA_ax = deque(maxlen=MAX_POINTS)
imuA_ay = deque(maxlen=MAX_POINTS)
imuA_az = deque(maxlen=MAX_POINTS)

imuB_ax = deque(maxlen=MAX_POINTS)
imuB_ay = deque(maxlen=MAX_POINTS)
imuB_az = deque(maxlen=MAX_POINTS)

# ==== PLOTTING SETUP ====
plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1)

# IMU A lines
lineA_x, = ax1.plot([], [], label='ax')
lineA_y, = ax1.plot([], [], label='ay')
lineA_z, = ax1.plot([], [], label='az')
ax1.set_title("IMU 0x6A")
ax1.legend()

# IMU B lines
lineB_x, = ax2.plot([], [], label='ax')
lineB_y, = ax2.plot([], [], label='ay')
lineB_z, = ax2.plot([], [], label='az')
ax2.set_title("IMU 0x6B")
ax2.legend()

def update_plot():
    # Update IMU A
    lineA_x.set_data(range(len(imuA_ax)), imuA_ax)
    lineA_y.set_data(range(len(imuA_ay)), imuA_ay)
    lineA_z.set_data(range(len(imuA_az)), imuA_az)
    ax1.relim()
    ax1.autoscale_view()

    # Update IMU B
    lineB_x.set_data(range(len(imuB_ax)), imuB_ax)
    lineB_y.set_data(range(len(imuB_ay)), imuB_ay)
    lineB_z.set_data(range(len(imuB_az)), imuB_az)
    ax2.relim()
    ax2.autoscale_view()

    plt.pause(0.01)

print("Listening to serial...")

# ==== MAIN LOOP ====
while True:
    try:
        line = ser.readline().decode('utf-8').strip()

        # Skip header or empty lines
        if not line or line.startswith("imu"):
            continue

        parts = line.split(',')

        if len(parts) != 8:
            continue

        imu_id = parts[0]
        ax_val = float(parts[1])
        ay_val = float(parts[2])
        az_val = float(parts[3])

        # Route to correct IMU
        if imu_id == "IMU_0x6A":
            imuA_ax.append(ax_val)
            imuA_ay.append(ay_val)
            imuA_az.append(az_val)

        elif imu_id == "IMU_0x6B":
            imuB_ax.append(ax_val)
            imuB_ay.append(ay_val)
            imuB_az.append(az_val)

        update_plot()

    except Exception as e:
        print("Error:", e)