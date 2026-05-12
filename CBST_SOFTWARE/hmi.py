import io
import numpy as np
import pandas as pd
from flask import Flask, jsonify, render_template, request

WRIST_ID = "0x6A"
SHOULDER_ID = "0x6B"

SMOOTH_WINDOW = 10

app = Flask(__name__)


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/upload", methods=["POST"])
def upload():
    if "file" not in request.files:
        return jsonify(error="No file part in request"), 400

    f = request.files["file"]
    if f.filename == "":
        return jsonify(error="No file selected"), 400

    try:
        df = pd.read_csv(io.StringIO(f.read().decode("utf-8")))
    except Exception as e:
        return jsonify(error=f"Failed to parse CSV: {e}"), 400

    required = {"timestamp_ms", "imu", "ax", "ay", "az", "gx", "gy", "gz"}
    missing = required - set(df.columns)

    if missing:
        return jsonify(error=f"CSV missing columns: {sorted(missing)}"), 400

    df["time_s"] = (
        df["timestamp_ms"] - df["timestamp_ms"].iloc[0]
    ) / 1000.0

    return jsonify(
        wrist=_imu_payload(df, WRIST_ID),
        shoulder=_imu_payload(df, SHOULDER_ID),
    )


def calculate_calibrated_angle(time_s, gx):
    calibrated_angle = []
    angle = 0.0

    previous_time = None

    for current_time, gx_value in zip(time_s, gx):
        if previous_time is not None:
            dt = current_time - previous_time

            # Convert gyroscope x-axis angular velocity from rad/s to degrees
            angle += gx_value * dt * (180.0 / np.pi)

        # Wrap angle between 0 and 360 degrees
        calibrated_angle.append(angle % 360.0)

        previous_time = current_time

    return calibrated_angle


def calculate_release_analysis(sub):
    """
    Compute smoothed magnitudes, the cumulative gyroscope angle on the
    dominant rotation axis, and the values sampled at the button-release
    moment (which is the final row of the recording).
    """
    sub = sub.copy().reset_index(drop=True)

    # Acceleration and gyroscope magnitudes
    sub["accel_mag"] = np.sqrt(
        sub["ax"] ** 2 + sub["ay"] ** 2 + sub["az"] ** 2
    )
    sub["gyro_mag"] = np.sqrt(
        sub["gx"] ** 2 + sub["gy"] ** 2 + sub["gz"] ** 2
    )

    # Smooth to reduce noise
    sub["accel_mag_smooth"] = (
        sub["accel_mag"]
        .rolling(SMOOTH_WINDOW, center=True, min_periods=1)
        .mean()
    )
    sub["gyro_mag_smooth"] = (
        sub["gyro_mag"]
        .rolling(SMOOTH_WINDOW, center=True, min_periods=1)
        .mean()
    )

    for col in ["gx", "gy", "gz"]:
        sub[f"{col}_smooth"] = (
            sub[col]
            .rolling(SMOOTH_WINDOW, center=True, min_periods=1)
            .mean()
        )

    # Detect dominant gyroscope axis for swing rotation.
    # Using the signed axis with the highest variance avoids the
    # magnitude-accumulation problem where gyro_mag (always positive)
    # gives inflated angles.
    gyro_axis_vars = {
        col: sub[f"{col}_smooth"].var() for col in ["gx", "gy", "gz"]
    }
    primary_gyro_axis = max(gyro_axis_vars, key=gyro_axis_vars.get)

    dt = sub["time_s"].diff().fillna(0)

    sub["gyro_angle_rad"] = np.cumsum(
        sub[f"{primary_gyro_axis}_smooth"].values * dt.values
    )
    sub["gyro_angle_deg"] = np.degrees(sub["gyro_angle_rad"])

    # Release moment is the last row (button release stops the recording)
    release_pos = sub.index[-1]

    release_angle = abs(sub.loc[release_pos, "gyro_angle_deg"])
    release_accel = sub.loc[release_pos, "accel_mag_smooth"]
    release_gyro = sub.loc[release_pos, "gyro_mag_smooth"]
    release_time = sub.loc[release_pos, "time_s"]

    return {
        "accel_mag_smooth": sub["accel_mag_smooth"].tolist(),
        "gyro_mag_smooth": sub["gyro_mag_smooth"].tolist(),
        "gyro_angle_deg": sub["gyro_angle_deg"].tolist(),
        "primary_gyro_axis": primary_gyro_axis,
        "release": {
            "angle_deg": float(release_angle),
            "accel_mag": float(release_accel),
            "gyro_mag": float(release_gyro),
            "time_s": float(release_time),
        },
    }


def _imu_payload(df, imu_id):
    sub = df[df["imu"] == imu_id].copy()

    time_s = sub["time_s"].tolist()
    gx = sub["gx"].tolist()

    calibrated_angle = calculate_calibrated_angle(time_s, gx)

    payload = {
        "time_s": time_s,

        "ax": sub["ax"].tolist(),
        "ay": sub["ay"].tolist(),
        "az": sub["az"].tolist(),

        "gx": sub["gx"].tolist(),
        "gy": sub["gy"].tolist(),
        "gz": sub["gz"].tolist(),

        "calibrated_angle": calibrated_angle,
    }

    # Only the shoulder gets the full release analysis
    if imu_id == SHOULDER_ID and len(sub) > 0:
        payload["release_analysis"] = calculate_release_analysis(sub)

    return payload


if __name__ == "__main__":
    app.run(debug=True)