import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ================================
# Load CSV files
# ================================
csv_file = "too_high.csv"
comparison_file = "reference.csv"

df = pd.read_csv(csv_file)
comp = pd.read_csv(comparison_file)

# ================================
# Create Data Plot
# ================================
fig, axs = plt.subplots(df, sharex=False)