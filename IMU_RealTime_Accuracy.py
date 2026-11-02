
import pandas as pd
from pathlib import Path

from sklearn.metrics import (
    accuracy_score,
    classification_report,
    confusion_matrix
)

# 1. LOAD REAL-TIME TEST DATA

RESULTS_DIR = Path(__file__).resolve().parent / "imu_results"

# Find all labeled real-time prediction files
csv_files = sorted(RESULTS_DIR.glob("realtime_predictions_*.csv"))

dataframes = []

for file in csv_files:
    df = pd.read_csv(file)

    # Skip earlier CSV files without actual activity labels
    if "actual_activity" not in df.columns:
        continue

    if df.empty:
        continue

    dataframes.append(df)

    print(f"Loaded: {file.name} ({len(df)} windows)")

if not dataframes:
    raise ValueError("No labeled real-time prediction files found.")

# Combine all test recordings
data = pd.concat(dataframes, ignore_index=True)

print("\nTotal windows:", len(data))

# 2. CALCULATE ACCURACY

actual = data["actual_activity"]
raw = data["raw_prediction"]
smoothed = data["smoothed_prediction"]

raw_accuracy = accuracy_score(actual, raw)
smoothed_accuracy = accuracy_score(actual, smoothed)

print("\nREAL-TIME CLASSIFICATION ACCURACY")
print(f"Raw accuracy: {raw_accuracy:.4f}")
print(f"Smoothed accuracy: {smoothed_accuracy:.4f}")

# 3. CLASSIFICATION REPORT

print("\nRAW CLASSIFICATION REPORT")
print(classification_report(actual, raw, zero_division=0))

print("\nSMOOTHED CLASSIFICATION REPORT")
print(classification_report(actual, smoothed, zero_division=0))


# 4. CONFUSION MATRICES

labels = [
    "sitting",
    "standing",
    "walking",
    "stairs_up",
    "stairs_down"
]

print("\nRAW CONFUSION MATRIX")
print(confusion_matrix(actual, raw, labels=labels))

print("\nSMOOTHED CONFUSION MATRIX")
print(confusion_matrix(actual, smoothed, labels=labels))

print("\nClass order:", labels)
