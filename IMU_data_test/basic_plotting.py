import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque

PORT = "COM6"
BAUD = 115200
MAX_POINTS = 300
READ_TIMEOUT = 0.1

# Open serial port
ser = serial.Serial(PORT, BAUD, timeout=READ_TIMEOUT)

# Give the port a moment to settle
plt.pause(0.5)

# Data buffers
ax_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
ay_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
az_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
gx_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
gy_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
gz_vals = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)

# Optional: track whether we have received any valid data yet
have_data = False

# Set up plot
fig, ax = plt.subplots()
x_vals = list(range(MAX_POINTS))

line_ax, = ax.plot(x_vals, list(ax_vals), label="Accel X")
line_ay, = ax.plot(x_vals, list(ay_vals), label="Accel Y")
line_az, = ax.plot(x_vals, list(az_vals), label="Accel Z")
line_gx, = ax.plot(x_vals, list(gx_vals), label="Gyro X")
line_gy, = ax.plot(x_vals, list(gy_vals), label="Gyro Y")
line_gz, = ax.plot(x_vals, list(gz_vals), label="Gyro Z")

ax.set_title("Live IMU Data")
ax.set_xlabel("Recent Samples")
ax.set_ylabel("Value")
ax.legend()
ax.grid(True)

def parse_csv_line(line: str):
    """
    Expected format:
    ax,ay,az,gx,gy,gz,temp

    Returns list of 7 floats, or None if invalid.
    """
    parts = line.strip().split(",")
    if len(parts) != 7:
        return None

    try:
        return [float(p) for p in parts]
    except ValueError:
        return None

def update_plot_lines():
    line_ax.set_ydata(list(ax_vals))
    line_ay.set_ydata(list(ay_vals))
    line_az.set_ydata(list(az_vals))
    line_gx.set_ydata(list(gx_vals))
    line_gy.set_ydata(list(gy_vals))
    line_gz.set_ydata(list(gz_vals))

def autoscale_y():
    all_y = (
        list(ax_vals) + list(ay_vals) + list(az_vals) +
        list(gx_vals) + list(gy_vals) + list(gz_vals)
    )

    y_min = min(all_y)
    y_max = max(all_y)

    if y_min == y_max:
        y_min -= 1.0
        y_max += 1.0
    else:
        pad = 0.1 * (y_max - y_min)
        y_min -= pad
        y_max += pad

    ax.set_ylim(y_min, y_max)

def update(frame):
    global have_data

    got_new_data = False

    # Read all currently buffered serial lines
    while ser.in_waiting > 0:
        try:
            raw = ser.readline().decode("utf-8", errors="ignore").strip()
        except Exception:
            continue

        if not raw:
            continue

        values = parse_csv_line(raw)
        if values is None:
            # Ignore non-CSV debug/setup lines
            continue

        ax_v, ay_v, az_v, gx_v, gy_v, gz_v, _temp = values

        ax_vals.append(ax_v)
        ay_vals.append(ay_v)
        az_vals.append(az_v)
        gx_vals.append(gx_v)
        gy_vals.append(gy_v)
        gz_vals.append(gz_v)

        got_new_data = True
        have_data = True

    if got_new_data:
        update_plot_lines()
        autoscale_y()

        if not ax.get_title().startswith("Live IMU Data"):
            ax.set_title("Live IMU Data")

    elif not have_data:
        ax.set_title("Live IMU Data (waiting for serial data...)")

    return line_ax, line_ay, line_az, line_gx, line_gy, line_gz

def on_close(event):
    try:
        if ser.is_open:
            ser.close()
    except Exception:
        pass

fig.canvas.mpl_connect("close_event", on_close)

ani = FuncAnimation(
    fig,
    update,
    interval=50,
    blit=False,
    cache_frame_data=False
)

plt.show()

# Fallback close
try:
    if ser.is_open:
        ser.close()
except Exception:
    pass