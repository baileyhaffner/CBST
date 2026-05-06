import subprocess
import sys

PORT = "COM6" # Use Bailey's computer on COM6 

# 1. Run transfer.py
subprocess.run(
    [sys.executable, "transfer.py", "--port", PORT],
    check=True
)

# 2. Run analysis.py
subprocess.run(
    [sys.executable, "hmi.py"],
    check=True
)