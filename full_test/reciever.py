"""
receiver.py
===========
Connects to the ESP32 TCP server and retrieves buffered IMU data as CSV.

Usage:
    python receiver.py                         # fetch once, print to console
    python receiver.py --save                  # fetch once, save to timestamped CSV
    python receiver.py --stream --save         # fetch repeatedly, append to same file

Requirements:
    pip install pandas
"""

import socket
import argparse
import time
from datetime import datetime
from io import StringIO

import pandas as pd

# ---------------------------------------------------------------
#  EDIT THIS to match your ESP32's IP (printed on serial monitor)
# ---------------------------------------------------------------
ESP32_IP   = "192.168.x.x"
ESP32_PORT = 5005
# ---------------------------------------------------------------

TIMEOUT_S       = 10     # seconds to wait for a response
STREAM_INTERVAL = 5      # seconds between fetches in --stream mode

# Expected CSV columns — must match data_buffer.h header
EXPECTED_COLUMNS = ["ax", "ay", "az", "gx", "gy", "gz", "temp"]


def fetch_data(ip: str, port: int) -> pd.DataFrame | None:
    """
    Open a TCP connection, send GET, read CSV rows until END, return DataFrame.
    Returns None if connection fails or no data received.
    """
    try:
        with socket.create_connection((ip, port), timeout=TIMEOUT_S) as sock:
            sock.sendall(b"GET\n")

            raw = ""
            buffer = ""
            while True:
                chunk = sock.recv(4096).decode("utf-8", errors="replace")
                if not chunk:
                    break
                buffer += chunk
                if "END" in buffer:
                    raw = buffer[:buffer.index("END")]
                    break

        if not raw.strip():
            print("[receiver] No data returned from ESP32.")
            return None

        df = pd.read_csv(StringIO(raw))

        # Validate columns
        if list(df.columns) != EXPECTED_COLUMNS:
            print(f"[receiver] Unexpected columns: {list(df.columns)}")
            return None

        return df

    except (ConnectionRefusedError, TimeoutError, OSError) as e:
        print(f"[receiver] Connection error: {e}")
        return None


def ping(ip: str, port: int) -> bool:
    """Send PING and check for PONG — useful to verify ESP is reachable."""
    try:
        with socket.create_connection((ip, port), timeout=5) as sock:
            sock.sendall(b"PING\n")
            reply = sock.recv(16).decode().strip()
            return reply == "PONG"
    except OSError:
        return False


def save_csv(df: pd.DataFrame, path: str, append: bool = False) -> None:
    mode   = "a" if append else "w"
    header = not append
    df.to_csv(path, mode=mode, header=header, index=False)
    print(f"[receiver] Saved {len(df)} rows → {path} (append={append})")


def main():
    parser = argparse.ArgumentParser(description="Fetch IMU data from ESP32 over TCP.")
    parser.add_argument("--save",   action="store_true", help="Save data to CSV file.")
    parser.add_argument("--stream", action="store_true", help="Fetch repeatedly on interval.")
    parser.add_argument("--ping",   action="store_true", help="Ping ESP32 and exit.")
    args = parser.parse_args()

    if args.ping:
        ok = ping(ESP32_IP, ESP32_PORT)
        print(f"[receiver] PING → {'PONG ✓' if ok else 'No response ✗'}")
        return

    # Build a timestamped filename once per session
    filename = f"imu_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv" if args.save else None
    first_write = True

    while True:
        print(f"[receiver] Fetching from {ESP32_IP}:{ESP32_PORT} ...")
        df = fetch_data(ESP32_IP, ESP32_PORT)

        if df is not None and not df.empty:
            print(df.to_string(index=False))
            print(f"[receiver] {len(df)} samples received.")

            if filename:
                save_csv(df, filename, append=not first_write)
                first_write = False
        else:
            print("[receiver] No data.")

        if not args.stream:
            break

        print(f"[receiver] Waiting {STREAM_INTERVAL}s before next fetch...\n")
        time.sleep(STREAM_INTERVAL)


if __name__ == "__main__":
    main()