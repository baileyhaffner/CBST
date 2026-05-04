import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ================================
# Load CSV files
# ================================
csv_file = "too_high.csv"
df = pd.read_csv(csv_file)

# ================================
# Create Data Plot
# ================================
fig, axs = plt.plot(df)
plt.show(axs)