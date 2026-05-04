import io

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

    df["time_s"] = (df["timestamp_ms"] - df["timestamp_ms"].iloc[0]) / 1000.0

    return jsonify(
        wrist=_imu_payload(df, WRIST_ID),
        shoulder=_imu_payload(df, SHOULDER_ID),
    )


def _imu_payload(df, imu_id):
    sub = df[df["imu"] == imu_id]
    return {
        "time_s": sub["time_s"].tolist(),
        "ax": sub["ax"].tolist(),
        "ay": sub["ay"].tolist(),
        "az": sub["az"].tolist(),
        "gx": sub["gx"].tolist(),
        "gy": sub["gy"].tolist(),
        "gz": sub["gz"].tolist(),
    }


if __name__ == "__main__":
    app.run(debug=True)
