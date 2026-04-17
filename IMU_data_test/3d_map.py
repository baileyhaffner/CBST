import serial
import time
import math
import numpy as np
import matplotlib.pyplot as plt
from collections import deque

PORT = "COM6"
BAUD = 115200
READ_TIMEOUT = 0.05

MAX_SIGNAL_POINTS = 300
MAX_TRAIL_POINTS = 1200

# ----------------------------
# Fixed plot scales
# ----------------------------
SIGNAL_Y_MIN = -25
SIGNAL_Y_MAX = 25

X_LIM = (-1.5, 1.5)
Y_LIM = (-1.5, 1.5)
Z_LIM = (-1.5, 1.5)

# ----------------------------
# Tuning
# ----------------------------
G = 9.81
ALPHA = 0.98                   # complementary filter strength
ACCEL_NOISE_DEADBAND = 0.12    # m/s^2
STATIONARY_GYRO_THRESH = 0.08  # rad/s
STATIONARY_ACC_THRESH = 0.35   # m/s^2 around |a| ~= g
VELOCITY_DAMPING = 0.999       # tiny damping
CALIBRATION_SAMPLES = 200

# ----------------------------
# Serial open
# ----------------------------
try:
    ser = serial.Serial(PORT, BAUD, timeout=READ_TIMEOUT)
except Exception as e:
    print(f"Failed to open {PORT}: {e}")
    raise SystemExit

plt.pause(0.5)

# ----------------------------
# Buffers for signal plotting
# ----------------------------
ax_vals = deque([0.0] * MAX_SIGNAL_POINTS, maxlen=MAX_SIGNAL_POINTS)
ay_vals = deque([0.0] * MAX_SIGNAL_POINTS, maxlen=MAX_SIGNAL_POINTS)
az_vals = deque([0.0] * MAX_SIGNAL_POINTS, maxlen=MAX_SIGNAL_POINTS)
gx_vals = deque([0.0] * MAX_SIGNAL_POINTS, maxlen=MAX_SIGNAL_POINTS)
gy_vals = deque([0.0] * MAX_SIGNAL_POINTS, maxlen=MAX_SIGNAL_POINTS)
gz_vals = deque([0.0] * MAX_SIGNAL_POINTS, maxlen=MAX_SIGNAL_POINTS)

# ----------------------------
# State
# ----------------------------
have_data = False
last_time = None

# Orientation (radians)
roll = 0.0
pitch = 0.0
yaw = 0.0

# Motion state in world frame
vel = np.zeros(3, dtype=float)
pos = np.zeros(3, dtype=float)

# 3D trail
pos_x = deque([0.0], maxlen=MAX_TRAIL_POINTS)
pos_y = deque([0.0], maxlen=MAX_TRAIL_POINTS)
pos_z = deque([0.0], maxlen=MAX_TRAIL_POINTS)

# Calibration storage
calibration_data = []
gyro_bias = np.zeros(3, dtype=float)
accel_rest_mean = np.zeros(3, dtype=float)
calibrated = False

# ----------------------------
# Plot setup
# ----------------------------
plt.ion()

# Figure 1: raw IMU traces
fig1, ax1 = plt.subplots()
x_vals = list(range(MAX_SIGNAL_POINTS))

line_ax, = ax1.plot(x_vals, list(ax_vals), label="Accel X")
line_ay, = ax1.plot(x_vals, list(ay_vals), label="Accel Y")
line_az, = ax1.plot(x_vals, list(az_vals), label="Accel Z")
line_gx, = ax1.plot(x_vals, list(gx_vals), label="Gyro X")
line_gy, = ax1.plot(x_vals, list(gy_vals), label="Gyro Y")
line_gz, = ax1.plot(x_vals, list(gz_vals), label="Gyro Z")

ax1.set_title("Live IMU Data")
ax1.set_xlabel("Recent Samples")
ax1.set_ylabel("Sensor Value")
ax1.set_ylim(SIGNAL_Y_MIN, SIGNAL_Y_MAX)
ax1.legend()
ax1.grid(True)

# Figure 2: 3D estimated position
fig2 = plt.figure()
ax2 = fig2.add_subplot(111, projection="3d")
trail_line, = ax2.plot(list(pos_x), list(pos_y), list(pos_z), label="Estimated Path")
point_3d = ax2.scatter([0.0], [0.0], [0.0], s=60)

ax2.set_title("Estimated 3D Position Map")
ax2.set_xlabel("X (m)")
ax2.set_ylabel("Y (m)")
ax2.set_zlabel("Z (m)")
ax2.set_xlim(*X_LIM)
ax2.set_ylim(*Y_LIM)
ax2.set_zlim(*Z_LIM)
ax2.legend()

print("Keep the IMU still during startup calibration...")

def parse_csv_line(line: str):
    """
    Expected:
    ax,ay,az,gx,gy,gz,temp
    """
    parts = line.strip().split(",")
    if len(parts) != 7:
        return None
    try:
        return [float(p) for p in parts]
    except ValueError:
        return None

def update_signal_lines():
    line_ax.set_ydata(list(ax_vals))
    line_ay.set_ydata(list(ay_vals))
    line_az.set_ydata(list(az_vals))
    line_gx.set_ydata(list(gx_vals))
    line_gy.set_ydata(list(gy_vals))
    line_gz.set_ydata(list(gz_vals))

def rotation_matrix_from_euler(roll, pitch, yaw):
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)

    rx = np.array([
        [1, 0, 0],
        [0, cr, -sr],
        [0, sr, cr]
    ])

    ry = np.array([
        [cp, 0, sp],
        [0, 1, 0],
        [-sp, 0, cp]
    ])

    rz = np.array([
        [cy, -sy, 0],
        [sy,  cy, 0],
        [0,   0,  1]
    ])

    return rz @ ry @ rx

def accel_to_roll_pitch(accel_vec):
    ax, ay, az = accel_vec
    roll_acc = math.atan2(ay, az)
    pitch_acc = math.atan2(-ax, math.sqrt(ay * ay + az * az))
    return roll_acc, pitch_acc

def is_stationary(accel_body, gyro_body):
    accel_mag = np.linalg.norm(accel_body)
    gyro_mag = np.linalg.norm(gyro_body)
    return (
        abs(accel_mag - G) < STATIONARY_ACC_THRESH
        and gyro_mag < STATIONARY_GYRO_THRESH
    )

def apply_deadband(vec, threshold):
    out = vec.copy()
    out[np.abs(out) < threshold] = 0.0
    return out

def update_3d_plot():
    global point_3d
    trail_line.set_data(list(pos_x), list(pos_y))
    trail_line.set_3d_properties(list(pos_z))
    point_3d.remove()
    point_3d = ax2.scatter([pos[0]], [pos[1]], [pos[2]], s=60)

try:
    while plt.fignum_exists(fig1.number) and plt.fignum_exists(fig2.number):
        got_new_data = False

        while ser.in_waiting > 0:
            try:
                raw = ser.readline().decode("utf-8", errors="ignore").strip()
            except Exception:
                continue

            if not raw:
                continue

            values = parse_csv_line(raw)
            if values is None:
                continue

            ax_v, ay_v, az_v, gx_v, gy_v, gz_v, _temp = values

            accel_body = np.array([ax_v, ay_v, az_v], dtype=float)
            gyro_body = np.array([gx_v, gy_v, gz_v], dtype=float)

            # Plot raw values
            ax_vals.append(ax_v)
            ay_vals.append(ay_v)
            az_vals.append(az_v)
            gx_vals.append(gx_v)
            gy_vals.append(gy_v)
            gz_vals.append(gz_v)

            now = time.time()
            if last_time is None:
                last_time = now
                calibration_data.append((accel_body.copy(), gyro_body.copy()))
                got_new_data = True
                have_data = True
                continue

            dt = now - last_time
            last_time = now

            if dt <= 0 or dt > 0.25:
                continue

            # ----------------------------
            # Startup calibration
            # ----------------------------
            if not calibrated:
                calibration_data.append((accel_body.copy(), gyro_body.copy()))

                if len(calibration_data) >= CALIBRATION_SAMPLES:
                    accel_samples = np.array([a for a, g in calibration_data])
                    gyro_samples = np.array([g for a, g in calibration_data])

                    accel_rest_mean[:] = np.mean(accel_samples, axis=0)
                    gyro_bias[:] = np.mean(gyro_samples, axis=0)

                    # Set initial roll and pitch from measured gravity vector
                    roll, pitch = accel_to_roll_pitch(accel_rest_mean)
                    yaw = 0.0

                    calibrated = True
                    print("\nCalibration complete.")
                    print(f"Initial roll={math.degrees(roll):.2f} deg, pitch={math.degrees(pitch):.2f} deg")
                    print(f"Gyro bias = {gyro_bias}")

                got_new_data = True
                have_data = True
                continue

            # ----------------------------
            # Bias-correct gyro
            # ----------------------------
            gyro_corr = gyro_body - gyro_bias

            # Gyro is already in rad/s for Adafruit Unified Sensor
            roll_gyro = roll + gyro_corr[0] * dt
            pitch_gyro = pitch + gyro_corr[1] * dt
            yaw = yaw + gyro_corr[2] * dt

            # ----------------------------
            # Complementary filter for roll/pitch
            # ----------------------------
            roll_acc, pitch_acc = accel_to_roll_pitch(accel_body)

            roll = ALPHA * roll_gyro + (1.0 - ALPHA) * roll_acc
            pitch = ALPHA * pitch_gyro + (1.0 - ALPHA) * pitch_acc

            # ----------------------------
            # Rotate accel to world frame
            # ----------------------------
            R = rotation_matrix_from_euler(roll, pitch, yaw)
            accel_world = R @ accel_body

            # Subtract gravity once
            accel_linear = accel_world - np.array([0.0, 0.0, G], dtype=float)

            # Reduce tiny noise
            accel_linear = apply_deadband(accel_linear, ACCEL_NOISE_DEADBAND)

            # ----------------------------
            # Stationary detection
            # ----------------------------
            if is_stationary(accel_body, gyro_corr):
                vel[:] = 0.0
            else:
                vel[:] = vel + accel_linear * dt
                vel[:] = vel * VELOCITY_DAMPING
                pos[:] = pos + vel * dt

            pos_x.append(pos[0])
            pos_y.append(pos[1])
            pos_z.append(pos[2])

            speed = np.linalg.norm(vel)
            print(
                f"speed={speed: .3f} m/s   pos=({pos[0]: .3f}, {pos[1]: .3f}, {pos[2]: .3f}) m",
                end="\r"
            )

            got_new_data = True
            have_data = True

        if got_new_data:
            update_signal_lines()
            update_3d_plot()

            if not calibrated:
                ax1.set_title(f"Live IMU Data (calibrating {len(calibration_data)}/{CALIBRATION_SAMPLES})")
            else:
                ax1.set_title("Live IMU Data")
        elif not have_data:
            ax1.set_title("Live IMU Data (waiting for serial data...)")

        fig1.canvas.draw_idle()
        fig2.canvas.draw_idle()
        plt.pause(0.02)

except KeyboardInterrupt:
    print("\nStopped by user.")

finally:
    try:
        if ser.is_open:
            ser.close()
    except Exception:
        pass