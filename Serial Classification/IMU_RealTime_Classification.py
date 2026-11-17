

import serial
import time
import csv
import joblib
import threading
import numpy as np
import pandas as pd

from datetime import datetime
from pathlib import Path
from collections import deque, Counter
from queue import Queue, Empty


# 1. REAL-TIME TEST SETTINGS

# None because this test contains multiple activities
ACTUAL_ACTIVITY = None

SERIAL_PORT = "COM3"
BAUD_RATE = 230400

WINDOW_DURATION_US = 500_000

SENSORS = [
    "ax", "ay", "az",
    "gx", "gy", "gz",
    "mx", "my", "mz"
]


# 2. LOAD TRAINED RANDOM FOREST MODEL

OUTPUT_DIR = Path(__file__).resolve().parent / "imu_results"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

MODEL_PATH = OUTPUT_DIR / "exoskeleton_activity_model_05s_5trials.joblib"

artifact = joblib.load(MODEL_PATH)

model = artifact["model"]
feature_names = artifact["features"]

print("Random Forest model loaded successfully!")
print("Number of features:", len(feature_names))


# 3. INITIALIZE PREDICTION SMOOTHING

prediction_history = deque(maxlen=3)
previous_prediction = None



# 4. INITIALIZE TRANSITION MARKERS

transition_queue = Queue()


def listen_for_transitions():

    while True:

        try:
            input()

        except EOFError:
            break

        # Record time immediately after Enter is pressed
        marker_time = time.perf_counter()
        marker_wall_time = datetime.now().isoformat(
            timespec="milliseconds"
        )

        transition_queue.put(
            (marker_time, marker_wall_time)
        )

        print("\nTRANSITION MARKED!")


# 5. CREATE CSV FILES

file_timestamp = datetime.now().strftime(
    "%Y%m%d_%H%M%S"
)

# Prediction CSV
CSV_PATH = (
    OUTPUT_DIR /
    f"realtime_predictions_{file_timestamp}.csv"
)

csv_file = open(
    CSV_PATH,
    "w",
    newline="",
    encoding="utf-8"
)

csv_writer = csv.writer(csv_file)

csv_writer.writerow([
    "timestamp",
    "actual_activity",
    "raw_prediction",
    "smoothed_prediction",
    "confidence",
    "samples",
    "processing_time_ms",
    "prediction_monotonic_time"
])

# Transition marker CSV
TRANSITION_CSV_PATH = (
    OUTPUT_DIR /
    f"transition_markers_{file_timestamp}.csv"
)

transition_file = open(
    TRANSITION_CSV_PATH,
    "w",
    newline="",
    encoding="utf-8"
)

transition_writer = csv.writer(transition_file)

transition_writer.writerow([
    "marker_number",
    "timestamp",
    "monotonic_time"
])

print("Saving predictions to:", CSV_PATH)
print("Saving transition markers to:", TRANSITION_CSV_PATH)


# 6. FEATURE EXTRACTION

def extract_features(window):

    data = np.array(window, dtype=float)

    features = {}

    for i, axis in enumerate(SENSORS):

        # Skip Arduino timestamp column
        values = data[:, i + 1]

        features[f"{axis}_mean"] = np.mean(values)
        features[f"{axis}_std"] = np.std(values, ddof=0)
        features[f"{axis}_rms"] = np.sqrt(
            np.mean(values ** 2)
        )

    return pd.DataFrame([features])


# 7. SAVE PENDING TRANSITION MARKERS

transition_count = 1


def save_pending_markers():

    global transition_count

    while True:

        try:
            marker_time, marker_wall_time = (
                transition_queue.get_nowait()
            )

        except Empty:
            break

        transition_writer.writerow([
            transition_count,
            marker_wall_time,
            marker_time
        ])

        transition_file.flush()

        print(
            f"Saved transition marker {transition_count}"
        )

        transition_count += 1


# 8. START KEYBOARD LISTENER

listener_thread = threading.Thread(
    target=listen_for_transitions,
    daemon=True
)

listener_thread.start()

print("\nPress ENTER immediately before each transition.")
print("Press CTRL+C to stop the recording.\n")


# 9. CONNECT TO ARDUINO

ser = None

try:

    ser = serial.Serial(
        SERIAL_PORT,
        BAUD_RATE,
        timeout=1
    )

    time.sleep(2)

    print("Connected to Arduino!")

    window = []
    window_start = None

    # 10. REAL-TIME CLASSIFICATION LOOP

    while True:

        # Save any keyboard transition markers
        save_pending_markers()

        line = ser.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()

        values = line.split()

        # Skip headers and invalid rows
        if len(values) != 10:
            continue

        try:
            sample = [float(v) for v in values]

        except ValueError:
            continue

        arduino_timestamp = sample[0]

        if window_start is None:
            window_start = arduino_timestamp

        window.append(sample)

        # 11. PROCESS EACH 1-SECOND WINDOW

        if (
            arduino_timestamp - window_start
            >= WINDOW_DURATION_US
        ):

            processing_start = time.perf_counter()

            print(
                f"Collected 1-second window: {len(window)} samples"
            )

            # Extract 27 features
            features = extract_features(window)

            print("Features extracted:", features.shape)

            # Match the original training feature order
            features = features[feature_names]

            # 12. RANDOM FOREST CLASSIFICATION

            # One inference call provides probabilities
            probabilities = model.predict_proba(
                features
            )[0]

            prediction = model.classes_[
                np.argmax(probabilities)
            ]

            confidence = float(
                np.max(probabilities)
            )

            # 13. MAJORITY-VOTE SMOOTHING

            prediction_history.append(prediction)

            counts = Counter(prediction_history)

            most_common, count = (
                counts.most_common(1)[0]
            )

            if count >= 2:

                smoothed_prediction = most_common

            elif len(prediction_history) == 1:

                smoothed_prediction = prediction

            else:

                smoothed_prediction = previous_prediction

            previous_prediction = smoothed_prediction

            # Record when the smoothed prediction is ready
            prediction_time = time.perf_counter()

            prediction_wall_time = datetime.now().isoformat(
                timespec="milliseconds"
            )

            # 14. MEASURE PROCESSING TIME

            processing_time_ms = (
                prediction_time - processing_start
            ) * 1000

            # 15. DISPLAY RESULTS

            print("Raw prediction:", prediction)
            print(
                "Smoothed prediction:",
                smoothed_prediction
            )
            print(
                f"Confidence: {confidence:.2%}"
            )
            print(
                f"Processing time: {processing_time_ms:.2f} ms"
            )
            print("-------------------------")

            # 16. SAVE PREDICTION TO CSV

            csv_writer.writerow([
                prediction_wall_time,
                ACTUAL_ACTIVITY,
                prediction,
                smoothed_prediction,
                round(confidence, 4),
                len(window),
                round(processing_time_ms, 3),
                prediction_time
            ])

            csv_file.flush()

            # 17. RESET WINDOW

            window = []
            window_start = None


# 18. STOP COLLECTION SAFELY

except KeyboardInterrupt:

    print("\nStopped collecting data.")

finally:

    # Save any remaining transition markers
    save_pending_markers()

    if ser is not None:
        ser.close()

    csv_file.close()
    transition_file.close()

    print("\nFiles saved successfully:")
    print("Predictions:", CSV_PATH)
    print("Transition markers:", TRANSITION_CSV_PATH)



# To run: [python "C:\Users\akals\OneDrive\Desktop\Lab\IMU_Analysis\IMU_RealTime_Classification.py"]