import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ================================
# Load CSV files
# ================================
csv_file = "data.csv"
comparison_file = "comparison.csv"

df = pd.read_csv(csv_file)
comp = pd.read_csv(comparison_file)

# ================================
# Reset time so both recordings start at 0 ms
# ================================
df["time_s"] = (df["timestamp_ms"] - df["timestamp_ms"].iloc[0]) / 1000
comp["time_s"] = (comp["timestamp_ms"] - comp["timestamp_ms"].iloc[0]) / 1000

# ================================
# Calculate acceleration and gyroscope magnitudes
# ================================
df["accel_mag"] = np.sqrt(df["ax"] ** 2 + df["ay"] ** 2 + df["az"] ** 2)
df["gyro_mag"] = np.sqrt(df["gx"] ** 2 + df["gy"] ** 2 + df["gz"] ** 2)

comp["accel_mag"] = np.sqrt(comp["ax"] ** 2 + comp["ay"] ** 2 + comp["az"] ** 2)
comp["gyro_mag"] = np.sqrt(comp["gx"] ** 2 + comp["gy"] ** 2 + comp["gz"] ** 2)

# ================================
# Smooth data to reduce noise
# ================================
smooth_window = 10

df["accel_mag_smooth"] = df["accel_mag"].rolling(smooth_window, center=True, min_periods=1).mean()
df["gyro_mag_smooth"] = df["gyro_mag"].rolling(smooth_window, center=True, min_periods=1).mean()

comp["accel_mag_smooth"] = comp["accel_mag"].rolling(smooth_window, center=True, min_periods=1).mean()
comp["gyro_mag_smooth"] = comp["gyro_mag"].rolling(smooth_window, center=True, min_periods=1).mean()

# ================================
# Automatically align recordings by peak gyroscope movement
# ================================
recorded_peak_time = df.loc[df["gyro_mag_smooth"].idxmax(), "time_s"]
comparison_peak_time = comp.loc[comp["gyro_mag_smooth"].idxmax(), "time_s"]

time_shift = recorded_peak_time - comparison_peak_time
comp["time_s_aligned"] = comp["time_s"] + time_shift

# ================================
# Interpolate comparison data onto recorded timestamps
# ================================
comparison_interp = pd.DataFrame()
comparison_interp["time_s"] = df["time_s"]

for column in ["accel_mag_smooth", "gyro_mag_smooth"]:
    comparison_interp[column] = np.interp(
        df["time_s"],
        comp["time_s_aligned"],
        comp[column]
    )

# ================================
# Create matched comparison dataframe
# ================================
matched = pd.DataFrame()
matched["time_s"] = df["time_s"]

matched["accel_recorded"] = df["accel_mag_smooth"]
matched["accel_comparison"] = comparison_interp["accel_mag_smooth"]

matched["gyro_recorded"] = df["gyro_mag_smooth"]
matched["gyro_comparison"] = comparison_interp["gyro_mag_smooth"]

# ================================
# Calculate percentage differences
# ================================
epsilon = 1e-6

matched["accel_percent_difference"] = (
    (matched["accel_recorded"] - matched["accel_comparison"]) /
    (matched["accel_comparison"] + epsilon)
) * 100

matched["gyro_percent_difference"] = (
    (matched["gyro_recorded"] - matched["gyro_comparison"]) /
    (matched["gyro_comparison"] + epsilon)
) * 100

# ================================
# Estimate release angle from gyroscope data
# ================================
matched["dt"] = matched["time_s"].diff().fillna(0)

matched["gyro_angle_recorded_rad"] = np.cumsum(matched["gyro_recorded"] * matched["dt"])
matched["gyro_angle_comparison_rad"] = np.cumsum(matched["gyro_comparison"] * matched["dt"])

matched["gyro_angle_recorded_deg"] = np.degrees(matched["gyro_angle_recorded_rad"])
matched["gyro_angle_comparison_deg"] = np.degrees(matched["gyro_angle_comparison_rad"])

release_angle_recorded = matched["gyro_angle_recorded_deg"].iloc[-1]
release_angle_comparison = matched["gyro_angle_comparison_deg"].iloc[-1]
release_angle_difference = release_angle_recorded - release_angle_comparison

# ================================
# Analyse first 50% of recording
# ================================
half_time = matched["time_s"].max() * 0.5
early_data = matched[matched["time_s"] <= half_time]

analysis_lines = []

analysis_lines.append(f"Recorded release angle: {release_angle_recorded:.1f} degrees")
analysis_lines.append(f"Comparison release angle: {release_angle_comparison:.1f} degrees")
analysis_lines.append(f"Release angle difference: {release_angle_difference:.1f} degrees")
analysis_lines.append("")

early_accel_difference = early_data["accel_percent_difference"].mean()
early_gyro_difference = early_data["gyro_percent_difference"].mean()

if early_accel_difference > 20:
    analysis_lines.append("Too much acceleration early.")
elif early_accel_difference < -20:
    analysis_lines.append("Not enough acceleration early.")
else:
    analysis_lines.append("Early acceleration is within 20% of comparison.")

if early_gyro_difference > 20:
    analysis_lines.append("Too much gyroscope movement early.")
elif early_gyro_difference < -20:
    analysis_lines.append("Not enough gyroscope movement early.")
else:
    analysis_lines.append("Early gyroscope movement is within 20% of comparison.")

analysis_lines.append("")
analysis_lines.append(f"Peak alignment shift applied: {time_shift:.3f} s")
analysis_lines.append(f"Average early acceleration difference: {early_accel_difference:.1f}%")
analysis_lines.append(f"Average early gyroscope difference: {early_gyro_difference:.1f}%")
analysis_lines.append(f"Overall acceleration difference: {matched['accel_percent_difference'].mean():.1f}%")
analysis_lines.append(f"Overall gyroscope difference: {matched['gyro_percent_difference'].mean():.1f}%")

analysis_text = "\n".join(analysis_lines)

# ================================
# Create one figure with three subplots
# ================================
fig, axs = plt.subplots(3, 1, figsize=(12, 10), sharex=False)

# ================================
# Acceleration comparison subplot
# ================================
axs[0].plot(matched["time_s"], matched["accel_recorded"], label="Acceleration recorded")
axs[0].plot(matched["time_s"], matched["accel_comparison"], label="Acceleration comparison")
axs[0].set_title("Acceleration Comparison")
axs[0].set_ylabel("Acceleration Magnitude (m/s^2)")
axs[0].legend()
axs[0].grid()

# ================================
# Gyroscope comparison subplot
# ================================
axs[1].plot(matched["time_s"], matched["gyro_recorded"], label="Gyroscope recorded")
axs[1].plot(matched["time_s"], matched["gyro_comparison"], label="Gyroscope comparison")
axs[1].set_title("Gyroscope Comparison")
axs[1].set_xlabel("Time (s)")
axs[1].set_ylabel("Gyroscope Magnitude (rad/s)")
axs[1].legend()
axs[1].grid()

# ================================
# Written analysis subplot
# ================================
axs[2].axis("off")
axs[2].set_title("Movement Analysis")
axs[2].text(
    0.02,
    0.95,
    analysis_text,
    transform=axs[2].transAxes,
    fontsize=11,
    verticalalignment="top"
)

# ================================
# Print summary
# ================================
print(analysis_text)

# ================================
# Show plot
# ================================
plt.tight_layout()
plt.show()

print("Test Complete")