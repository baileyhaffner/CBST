import io
import numpy as np
import pandas as pd
from flask import Flask, jsonify, render_template, request

WRIST_ID = "0x6A"
SHOULDER_ID = "0x6B"

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


def _imu_payload(df, imu_id):
    sub = df[df["imu"] == imu_id].copy()

    time_s = sub["time_s"].tolist()
    gx = sub["gx"].tolist()

    calibrated_angle = calculate_calibrated_angle(time_s, gx)

    return {
        "time_s": time_s,

        "ax": sub["ax"].tolist(),
        "ay": sub["ay"].tolist(),
        "az": sub["az"].tolist(),

        "gx": sub["gx"].tolist(),
        "gy": sub["gy"].tolist(),
        "gz": sub["gz"].tolist(),

        "calibrated_angle": calibrated_angle,
    }


if __name__ == "__main__":
    app.run(debug=True)