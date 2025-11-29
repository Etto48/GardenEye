import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import UnivariateSpline

# ------------------------
# PARAMETERS
# ------------------------
csv_file = "./embedded/battery/readings_01.csv"  # CSV with columns: timestamp,battery
example_voltage = 3.7     # Example battery voltage to estimate percentage

# ------------------------
# LOAD AND PREPARE DATA
# ------------------------
df = pd.read_csv(csv_file)

# Convert timestamp to datetime
df["timestamp"] = pd.to_datetime(df["timestamp"])
df = df.sort_values("timestamp").reset_index(drop=True)

# Convert timestamp to numeric (seconds since start)
start_time = df["timestamp"].iloc[0]
df["time_sec"] = (df["timestamp"] - start_time).dt.total_seconds()

# Normalize time: first=100%, last=0%
t_min, t_max = df["time_sec"].iloc[0], df["time_sec"].iloc[-1]
df["time_pct"] = 100 * (t_max - df["time_sec"]) / (t_max - t_min)

# Sort x and y together
idx = np.argsort(df["battery"])
x_sorted, y_sorted = df["battery"].values[idx], df["time_pct"].values[idx]

# Optionally remove duplicate x values (average their y)
x_unique, indices = np.unique(x_sorted, return_index=True)
y_unique = y_sorted[indices]

# ------------------------
# FIT SPLINE
# ------------------------
# Fit a univariate spline: battery voltage (x) -> time percentage (y)
spline = UnivariateSpline(x_unique, y_unique, s=10000, k=3, ext=0)  # s is smoothing factor

# Example usage
est_pct = spline(example_voltage)
print(f"Estimated percentage for voltage {example_voltage}V: {est_pct:.2f}%")

# ------------------------
# PLOT FIT
# ------------------------
b_fit = np.linspace(df["battery"].min(), df["battery"].max(), 200)
pct_fit = spline(b_fit)

plt.scatter(df["time_pct"], df["battery"], label="Data", color="blue")
plt.plot(pct_fit, b_fit, color="red", label="Inverse Fit")
plt.xlabel("Time % Remaining")
plt.ylabel("Battery Voltage")
plt.title("Battery Voltage → Time Percentage")
plt.legend()
plt.gca().invert_xaxis()
plt.show()
