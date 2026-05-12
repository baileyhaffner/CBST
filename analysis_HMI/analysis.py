import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# ================================
# Load CSV files
# ================================
_dir = os.path.dirname(os.path.abspath(__file__))
csv_file = os.path.join(_dir, "comparison.csv")
comparison_file = os.path.join(_dir, "comparison.csv")

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

for col in ["ax", "ay", "az", "gx", "gy", "gz"]:
    df[f"{col}_smooth"] = df[col].rolling(smooth_window, center=True, min_periods=1).mean()
    comp[f"{col}_smooth"] = comp[col].rolling(smooth_window, center=True, min_periods=1).mean()

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

for column in ["accel_mag_smooth", "gyro_mag_smooth",
               "ax_smooth", "ay_smooth", "az_smooth",
               "gx_smooth", "gy_smooth", "gz_smooth"]:
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

for axis in ["ax", "ay", "az"]:
    matched[f"{axis}_recorded"] = df[f"{axis}_smooth"].values
    matched[f"{axis}_comparison"] = comparison_interp[f"{axis}_smooth"].values

for axis in ["gx", "gy", "gz"]:
    matched[f"{axis}_recorded"] = df[f"{axis}_smooth"].values
    matched[f"{axis}_comparison"] = comparison_interp[f"{axis}_smooth"].values

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
# Detect dominant gyroscope axis for swing rotation
# ================================
# Using the signed axis with highest variance avoids the magnitude-accumulation
# problem where gyro_mag (always positive) gives 400+ degrees.
gyro_axis_vars = {col: df[f"{col}_smooth"].var() for col in ["gx", "gy", "gz"]}
primary_gyro_axis = max(gyro_axis_vars, key=gyro_axis_vars.get)

matched["dt"] = matched["time_s"].diff().fillna(0)

matched["gyro_angle_recorded_rad"] = np.cumsum(
    df[f"{primary_gyro_axis}_smooth"].values * matched["dt"].values
)
matched["gyro_angle_recorded_deg"] = np.degrees(matched["gyro_angle_recorded_rad"])

matched["gyro_angle_comparison_rad"] = np.cumsum(
    comparison_interp[f"{primary_gyro_axis}_smooth"].values * matched["dt"].values
)
matched["gyro_angle_comparison_deg"] = np.degrees(matched["gyro_angle_comparison_rad"])

# ================================
# Detect release moment (peak gyroscope magnitude)
# ================================
release_pos = matched["gyro_recorded"].idxmax()
comp_release_pos = matched["gyro_comparison"].idxmax()

release_angle_recorded = abs(matched.loc[release_pos, "gyro_angle_recorded_deg"])
release_angle_comparison = abs(matched.loc[comp_release_pos, "gyro_angle_comparison_deg"])
release_angle_difference = release_angle_recorded - release_angle_comparison

release_accel_recorded = matched.loc[release_pos, "accel_recorded"]
release_accel_comparison = matched.loc[comp_release_pos, "accel_comparison"]
release_accel_diff_pct = (
    (release_accel_recorded - release_accel_comparison) /
    (release_accel_comparison + epsilon)
) * 100

release_gyro_recorded = matched.loc[release_pos, "gyro_recorded"]
release_gyro_comparison = matched.loc[comp_release_pos, "gyro_comparison"]

# ================================
# Analyse first 50% of recording
# ================================
half_time = matched["time_s"].max() * 0.5
early_data = matched[matched["time_s"] <= half_time]

analysis_lines = []

analysis_lines.append(f"--- Release Angle (from vertical, axis: {primary_gyro_axis}) ---")
analysis_lines.append(f"Recorded:   {release_angle_recorded:.1f} degrees")
analysis_lines.append(f"Comparison: {release_angle_comparison:.1f} degrees")
analysis_lines.append(f"Difference: {release_angle_difference:.1f} degrees")
analysis_lines.append("")
analysis_lines.append("--- Acceleration at Release ---")
analysis_lines.append(f"Recorded:   {release_accel_recorded:.2f} m/s^2")
analysis_lines.append(f"Comparison: {release_accel_comparison:.2f} m/s^2")
analysis_lines.append(f"Difference: {release_accel_diff_pct:.1f}%")
analysis_lines.append("")
analysis_lines.append("--- Angular Velocity at Release ---")
analysis_lines.append(f"Recorded:   {release_gyro_recorded:.2f} rad/s")
analysis_lines.append(f"Comparison: {release_gyro_comparison:.2f} rad/s")
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
analysis_lines.append(f"Avg early accel difference:   {early_accel_difference:.1f}%")
analysis_lines.append(f"Avg early gyro difference:    {early_gyro_difference:.1f}%")
analysis_lines.append(f"Overall accel difference:     {matched['accel_percent_difference'].mean():.1f}%")
analysis_lines.append(f"Overall gyro difference:      {matched['gyro_percent_difference'].mean():.1f}%")

analysis_text = "\n".join(analysis_lines)

# ================================
# Figure 1: Magnitude comparison + analysis
# ================================
fig, axs = plt.subplots(3, 1, figsize=(12, 10), sharex=False)

axs[0].plot(matched["time_s"], matched["accel_recorded"], label="Recorded")
axs[0].plot(matched["time_s"], matched["accel_comparison"], label="Comparison")
axs[0].set_title("Acceleration Magnitude Comparison")
axs[0].set_ylabel("Acceleration Magnitude (m/s^2)")
axs[0].legend()
axs[0].grid()

axs[1].plot(matched["time_s"], matched["gyro_recorded"], label="Recorded")
axs[1].plot(matched["time_s"], matched["gyro_comparison"], label="Comparison")
axs[1].set_title("Gyroscope Magnitude Comparison")
axs[1].set_xlabel("Time (s)")
axs[1].set_ylabel("Gyroscope Magnitude (rad/s)")
axs[1].legend()
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
# Figure 2: XYZ axis breakdown
# ================================
fig2, axs2 = plt.subplots(3, 2, figsize=(14, 10), sharex=False)
fig2.suptitle("Acceleration and Gyroscope XYZ Axes")

for i, (acol, gcol, label) in enumerate(zip(["ax", "ay", "az"], ["gx", "gy", "gz"], ["X", "Y", "Z"])):
    axs2[i, 0].plot(matched["time_s"], matched[f"{acol}_recorded"], label="Recorded")
    axs2[i, 0].plot(matched["time_s"], matched[f"{acol}_comparison"], label="Comparison")
    axs2[i, 0].set_title(f"Acceleration {label}")
    axs2[i, 0].set_ylabel("Acceleration (m/s^2)")
    axs2[i, 0].legend()
    axs2[i, 0].grid()

    axs2[i, 1].plot(matched["time_s"], matched[f"{gcol}_recorded"], label="Recorded")
    axs2[i, 1].plot(matched["time_s"], matched[f"{gcol}_comparison"], label="Comparison")
    axs2[i, 1].set_title(f"Gyroscope {label}{'  ← primary swing axis' if gcol == primary_gyro_axis else ''}")
    axs2[i, 1].set_ylabel("Angular velocity (rad/s)")
    axs2[i, 1].legend()
    axs2[i, 1].grid()

axs2[2, 0].set_xlabel("Time (s)")
axs2[2, 1].set_xlabel("Time (s)")

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
