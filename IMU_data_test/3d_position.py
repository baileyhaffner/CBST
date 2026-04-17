import serial
import time
import math
import numpy as np
import matplotlib.pyplot as plt
from collections import deque

PORT = "COM6"
BAUD = 115200
READ_TIMEOUT = 0.05
MAX_POINTS = 300
MAX_TRAIL_POINTS = 1000

# ----------------------------
# FIXED DISPLAY LIMITS
# ----------------------------
SIGNAL_Y_MIN = -20
SIGNAL_Y_MAX = 20

X_LIM = (-2.0, 2.0)
Y_LIM = (-2.0, 2.0)
Z_LIM = (-2.0, 2.0)

# ----------------------------
# FILTER / INTEGRATION SETTINGS
# ----------------------------
ACCEL_DEADBAND = 0.15      # m/s^2; values smaller than this become 0
VELOCITY_DAMPING = 0.995   # mild damping to reduce drift
GYRO_DEG_TO_RAD = math.pi / 180.0

# ----------------------------
# Open serial port
# ----------------------------
try:
    ser = serial.Serial(PORT, BAUD, timeout=READ_TIMEOUT)
except Exception as e:
    print(f"Failed to open {PORT}: {e}")
    raise SystemExit

plt.pause(0.5)

# ----------------------------
# Buffers for raw display
# ----------------------------
ax_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
ay_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
az_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
gx_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
gy_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
gz_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)

# ----------------------------
# State variables
# ----------------------------
have_data = False
last_time = None

# Orientation in radians
roll = 0.0
pitch = 0.0
yaw = 0.0

# Velocity and position in world frame
vel = np.array([0.0, 0.0, 0.0], dtype=float)
pos = np.array([0.0, 0.0, 0.0], dtype=float)

# Trail
pos_x = deque([0.0], maxlen=MAX_TRAIL_POINTS)
pos_y = deque([0.0], maxlen=MAX_TRAIL_POINTS)
pos_z = deque([0.0], maxlen=MAX_TRAIL_POINTS)

# Bias calibration
calibration_samples = []
CALIBRATION_COUNT = 100
accel_bias = np.array([0.0, 0.0, 0.0], dtype=float)
bias_ready = False

# ----------------------------
# Plot setup
# ----------------------------
plt.ion()

# Figure 1: raw signals
fig1, ax1 = plt.subplots()
x_vals = list(range(MAX_POINTS))

line_ax, = ax1.plot(x_vals, list(ax_vals), label="Accel X")
line_ay, = ax1.plot(x_vals, list(ay_vals), label="Accel Y")
line_az, = ax1.plot(x_vals, list(az_vals), label="Accel Z")
line_gx, = ax1.plot(x_vals, list(gx_vals), label="Gyro X")
line_gy, = ax1.plot(x_vals, list(gy_vals), label="Gyro Y")
line_gz, = ax1.plot(x_vals, list(gz_vals), label="Gyro Z")

ax1.set_title("Live IMU Data")
ax1.set_xlabel("Recent Samples")
ax1.set_ylabel("Value")
ax1.set_ylim(SIGNAL_Y_MIN, SIGNAL_Y_MAX)
ax1.legend()
ax1.grid(True)

# Figure 2: 3D path
fig2 = plt.figure()
ax2 = fig2.add_subplot(111, projection="3d")
trail_line, = ax2.plot(list(pos_x), list(pos_y), list(pos_z), label="Position Trail")
point_3d = ax2.scatter([0.0], [0.0], [0.0], s=50)

ax2.set_title("Estimated 3D Position")
ax2.set_xlabel("X (m)")
ax2.set_ylabel("Y (m)")
ax2.set_zlabel("Z (m)")
ax2.set_xlim(*X_LIM)
ax2.set_ylim(*Y_LIM)
ax2.set_zlim(*Z_LIM)
ax2.legend()

# Optional text output in terminal
print("Starting live IMU position estimate...")
print("Keep the IMU still for initial calibration.")

def parse_csv_line(line: str):
    """
    Expected format:
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
    """
    ZYX rotation:
    R = Rz(yaw) @ Ry(pitch) @ Rx(roll)
    """
    cr = math.cos(roll)
    sr = math.sin(roll)
    cp = math.cos(pitch)
    sp = math.sin(pitch)
    cy = math.cos(yaw)
    sy = math.sin(yaw)

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
        [sy, cy, 0],
        [0, 0, 1]
    ])

    return rz @ ry @ rx

def apply_deadband(vec, threshold):
    out = vec.copy()
    for i in range(3):
        if abs(out[i]) < threshold:
            out[i] = 0.0
    return out

def update_3d_plot():
    global point_3d

    trail_line.set_data(list(pos_x), list(pos_y))
    trail_line.set_3d_properties(list(pos_z))

    point_3d.remove()
    point_3d = ax2.scatter([pos[0]], [pos[1]], [pos[2]], s=50)

def process_imu_sample(ax_v, ay_v, az_v, gx_v, gy_v, gz_v, dt):
    global roll, pitch, yaw, vel, pos

    # Gyro assumed deg/s -> rad/s
    gx_r = gx_v * GYRO_DEG_TO_RAD
    gy_r = gy_v * GYRO_DEG_TO_RAD
    gz_r = gz_v * GYRO_DEG_TO_RAD

    # Simple orientation integration
    roll += gx_r * dt
    pitch += gy_r * dt
    yaw += gz_r * dt

    # Raw accel in body frame
    accel_body = np.array([ax_v, ay_v, az_v], dtype=float)

    # Remove fixed bias estimated at startup
    accel_body = accel_body - accel_bias

    # Rotate into world frame
    R = rotation_matrix_from_euler(roll, pitch, yaw)
    accel_world = R @ accel_body

    # Remove gravity from world Z
    gravity_world = np.array([0.0, 0.0, 9.81], dtype=float)
    accel_world = accel_world - gravity_world

    # Small noise suppression
    accel_world = apply_deadband(accel_world, ACCEL_DEADBAND)

    # Integrate to velocity and position
    vel[:] = vel + accel_world * dt
    vel[:] = vel * VELOCITY_DAMPING
    pos[:] = pos + vel * dt

    # Save trail
    pos_x.append(pos[0])
    pos_y.append(pos[1])
    pos_z.append(pos[2])

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

            # Save raw data for 2D plots
            ax_vals.append(ax_v)
            ay_vals.append(ay_v)
            az_vals.append(az_v)
            gx_vals.append(gx_v)
            gy_vals.append(gy_v)
            gz_vals.append(gz_v)

            # Timing
            now = time.time()
            if last_time is None:
                last_time = now
                got_new_data = True
                have_data = True
                continue

            dt = now - last_time
            last_time = now

            if dt <= 0 or dt > 0.5:
                continue

            # Startup bias calibration
            accel_sample = np.array([ax_v, ay_v, az_v], dtype=float)
            if not bias_ready:
                calibration_samples.append(accel_sample)
                if len(calibration_samples) >= CALIBRATION_COUNT:
                    accel_bias[:] = np.mean(calibration_samples, axis=0)
                    bias_ready = True
                    print(f"Calibration complete. Accel bias = {accel_bias}")
                    print("Now move the IMU and watch the estimated 3D path.")
                got_new_data = True
                have_data = True
                continue

            # Process IMU data into velocity and position
            process_imu_sample(ax_v, ay_v, az_v, gx_v, gy_v, gz_v, dt)

            # Optional terminal output
            speed = np.linalg.norm(vel)
            print(
                f"pos=({pos[0]: .3f}, {pos[1]: .3f}, {pos[2]: .3f}) m   "
                f"speed={speed: .3f} m/s",
                end="\r"
            )

            got_new_data = True
            have_data = True

        if got_new_data:
            update_signal_lines()
            update_3d_plot()

            if not bias_ready:
                ax1.set_title(
                    f"Live IMU Data (calibrating: {len(calibration_samples)}/{CALIBRATION_COUNT})"
                )
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