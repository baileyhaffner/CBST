import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

CSV_FILE = "data.csv"

# Read CSV
df = pd.read_csv(CSV_FILE)

# Calculate vector magnitudes
df["accel_mag"] = np.sqrt(df["ax"]**2 + df["ay"]**2 + df["az"]**2)
df["gyro_mag"] = np.sqrt(df["gx"]**2 + df["gy"]**2 + df["gz"]**2)

# Split by IMU address
imu_a = df[df["imu"] == "0x6A"]
imu_b = df[df["imu"] == "0x6B"]

# Create 4 plots
fig, axs = plt.subplots(2, 2, figsize=(12, 8), sharex=False)

# IMU A acceleration
axs[0, 0].plot(imu_a["timestamp_ms"], imu_a["accel_mag"])
axs[0, 0].set_title("IMU A (0x6A) Acceleration Magnitude")
axs[0, 0].set_xlabel("Timestamp (ms)")
axs[0, 0].set_ylabel("Acceleration Magnitude")
axs[0, 0].grid(True)

# IMU A gyroscope
axs[0, 1].plot(imu_a["timestamp_ms"], imu_a["gyro_mag"])
axs[0, 1].set_title("IMU A (0x6A) Gyroscope Magnitude")
axs[0, 1].set_xlabel("Timestamp (ms)")
axs[0, 1].set_ylabel("Gyroscope Magnitude")
axs[0, 1].grid(True)

# IMU B acceleration
axs[1, 0].plot(imu_b["timestamp_ms"], imu_b["accel_mag"])
axs[1, 0].set_title("IMU B (0x6B) Acceleration Magnitude")
axs[1, 0].set_xlabel("Timestamp (ms)")
axs[1, 0].set_ylabel("Acceleration Magnitude")
axs[1, 0].grid(True)

# IMU B gyroscope
axs[1, 1].plot(imu_b["timestamp_ms"], imu_b["gyro_mag"])
axs[1, 1].set_title("IMU B (0x6B) Gyroscope Magnitude")
axs[1, 1].set_xlabel("Timestamp (ms)")
axs[1, 1].set_ylabel("Gyroscope Magnitude")
axs[1, 1].grid(True)

plt.tight_layout()
plt.show()
