import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

MAX_VOLTAGE = 4.2
MIN_VOLTAGE = 3.0
csv_file = "./embedded/battery/readings_01.csv"  # CSV with columns: timestamp,battery

def denoise_signal(x, window_length=1001):
    padded = np.pad(x, (window_length//2, window_length//2), mode='edge')
    return np.convolve(padded, np.ones(window_length)/window_length, mode='valid')

def plot_rescaled_curve():
    # Load data
    df = pd.read_csv(csv_file)
    battery = np.array(df["battery"].values)
    old_M = 2.0
    old_Q = 0.0
    # Undo old scaling
    battery = (battery - old_Q) / old_M

    max_battery = np.max(battery)
    min_battery = np.min(battery)
    # Rescale battery readings to percentage
    M_coeff = (MAX_VOLTAGE - MIN_VOLTAGE) / (max_battery - min_battery)
    Q_coeff = MIN_VOLTAGE - M_coeff * min_battery
    print(f"Rescaling coefficients: M = {M_coeff}, Q = {Q_coeff}")
    rescaled = M_coeff * battery + Q_coeff

    timestamp = pd.to_datetime(df["timestamp"])
    min_time = timestamp.iloc[-1]
    max_time = timestamp.iloc[0]
    duration = (max_time - min_time)
    sampling_interval = duration / len(timestamp)
    print(f"Time range: {min_time} to {max_time}\nDuration = {duration}\nSampling interval = {sampling_interval}")

    denoised = denoise_signal(rescaled)

    plt.plot(timestamp, denoised, label="Denoised Curve", color="green")
    plt.plot(timestamp, rescaled, label="Rescaled Data", color="green", alpha=0.5)
    plt.xlabel("Timestamp")
    plt.ylabel("Battery Voltage (V)")
    plt.title("Battery Voltage to Percentage Rescaling")
    plt.legend()
    plt.grid()
    plt.show()

if __name__ == "__main__":
    plot_rescaled_curve()