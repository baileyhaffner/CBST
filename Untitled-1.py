import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# ================================
# Load CSV files
# ================================
_dir = os.path.dirname(os.path.abspath(__file__))
csv_file = os.path.join(_dir, "good_fast.csv")

df = pd.read_csv(csv_file)

# ================================
# Reset time so recording starts at 0 ms
# ================================
df["time_s"] = (df["timestamp_ms"] - df["timestamp_ms"].iloc[0]) / 1000

# ================================
# Calculate acceleration and gyroscope magnitudes
# ================================
df["accel_mag"] = np.sqrt(df["ax"] ** 2 + df["ay"] ** 2 + df["az"] ** 2)
df["gyro_mag"] = np.sqrt(df["gx"] ** 2 + df["gy"] ** 2 + df["gz"] ** 2)

# ================================
# Smooth data to reduce noise
# ================================
smooth_window = 10

df["accel_mag_smooth"] = df["accel_mag"].rolling(smooth_window, center=True, min_periods=1).mean()
df["gyro_mag_smooth"] = df["gyro_mag"].rolling(smooth_window, center=True, min_periods=1).mean()

for col in ["gx", "gy", "gz"]:
    df[f"{col}_smooth"] = df[col].rolling(smooth_window, center=True, min_periods=1).mean()

# ================================
# Detect dominant gyroscope axis for swing rotation
# ================================
# Using the signed axis with highest variance avoids the magnitude-accumulation
# problem where gyro_mag (always positive) gives 400+ degrees.
gyro_axis_vars = {col: df[f"{col}_smooth"].var() for col in ["gx", "gy", "gz"]}
primary_gyro_axis = max(gyro_axis_vars, key=gyro_axis_vars.get)

dt = df["time_s"].diff().fillna(0)

df["gyro_angle_rad"] = np.cumsum(df[f"{primary_gyro_axis}_smooth"].values * dt.values)
df["gyro_angle_deg"] = np.degrees(df["gyro_angle_rad"])

# ================================
# Detect release moment (peak gyroscope magnitude)
# ================================
release_pos = df["gyro_mag_smooth"].idxmax()

release_angle = abs(df.loc[release_pos, "gyro_angle_deg"])
release_accel = df.loc[release_pos, "accel_mag_smooth"]
release_gyro  = df.loc[release_pos, "gyro_mag_smooth"]
release_time  = df.loc[release_pos, "time_s"]

# ================================
# Build analysis text
# ================================
analysis_lines = []

analysis_lines.append(f"--- Release Angle (from vertical, axis: {primary_gyro_axis}) ---")
analysis_lines.append(f"Release angle:    {release_angle:.1f} degrees")
analysis_lines.append("")
analysis_lines.append("--- Acceleration at Release ---")
analysis_lines.append(f"Acceleration:     {release_accel:.2f} m/s^2")
analysis_lines.append("")
analysis_lines.append("--- Angular Velocity at Release ---")
analysis_lines.append(f"Angular velocity: {release_gyro:.2f} rad/s")

analysis_text = "\n".join(analysis_lines)

# ================================
# Figure 1: Magnitude plots + analysis
# ================================
fig, axs = plt.subplots(3, 1, figsize=(12, 10), sharex=False)

axs[0].plot(df["time_s"], df["accel_mag_smooth"])
axs[0].set_title("Acceleration Magnitude")
axs[0].set_ylabel("Acceleration Magnitude (m/s^2)")
axs[0].grid()

axs[1].plot(df["time_s"], df["gyro_mag_smooth"])
axs[1].set_title("Gyroscope Magnitude")
axs[1].set_xlabel("Time (s)")
axs[1].set_ylabel("Gyroscope Magnitude (rad/s)")
axs[1].grid()

axs[2].axis("off")
axs[2].set_title("Movement Analysis")
axs[2].text(
    0.02,
    0.95,
    analysis_text,
    transform=axs[2].transAxes,
    fontsize=10,
    verticalalignment="top",
    family="monospace"
)

# ================================
# Print summary
# ================================
print(analysis_text)

# ================================
# Show plots
# ================================
plt.tight_layout()
plt.show()

print("Test Complete")
