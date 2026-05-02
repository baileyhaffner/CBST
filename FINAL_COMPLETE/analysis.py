import pandas as pd

csv_file = "data.csv"

df = pd.read_csv(csv_file)

print(df.head())
print(df.describe())

print("Test Complete")