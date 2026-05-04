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
result_graph = plt.plot(df)
