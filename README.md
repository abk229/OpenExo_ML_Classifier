# Real-Time Activity Recognition for the OpenExo Hip Exoskeleton

An embedded machine learning project that uses a wearable inertial measurement unit (IMU) to classify human activities and display live predictions in a Python graphical user interface (GUI).

## Project Overview

This research project explores activity recognition as a step toward context-aware assistance for a hip exoskeleton. An Arduino Nano 33 BLE Sense Rev2 collects IMU measurements, extracts statistical features from short windows, and runs a trained Random Forest classifier directly on the microcontroller.

The predicted activity and Random Forest tree-vote share are transmitted to a Python GUI using Bluetooth Low Energy (BLE). The GUI displays predictions in real time and allows labeled results to be recorded for evaluation.

**Scope:** The system recognizes and visualizes activities. Predictions do not control exoskeleton motors or adjust assistance settings.

## Recognized Activities

- Sitting
- Standing
- Walking
- Stairs up
- Stairs down

All five activities were included in offline evaluation. Live testing has focused on sitting, standing, walking, and transitions. Stair ascent and descent have not yet been fully validated live.

## Hardware and Software

### Hardware

- Arduino Nano 33 BLE Sense Rev2
- BMI270 accelerometer and gyroscope
- BMM150 magnetometer
- Computer with Bluetooth Low Energy support
- USB connection for programming and powering the Arduino

### Software

- Arduino / C++ for IMU acquisition, feature extraction, and embedded classification
- Python and scikit-learn for model development and evaluation
- PySide6 for the activity recognition GUI
- bleak for BLE communication
- pandas, NumPy, and Matplotlib for analysis and visualization

## Data Collection

The Arduino records timestamped, nine-axis IMU measurements:

`ax, ay, az, gx, gy, gz, mx, my, mz`

The accelerometer and gyroscope have native output rates of approximately 100 Hz; the magnetometer updates at approximately 10 Hz. The latest magnetometer reading is reused between updates. Actual recorded sample rates can be lower than the sensor's nominal rates.

The dataset contains **15 labeled trials**, with three trials for each of the five activities. The original recordings are stored in `Trial Data/`. The data collection sketch is stored in `Arduino/`.

## Machine Learning Pipeline

```text
9-axis IMU measurements
         |
         v
0.5-second windows (final embedded implementation)
         |
         v
27 statistical features
(mean, standard deviation, RMS for each axis)
         |
         v
Embedded Random Forest classifier (200 trees)
         |
         v
Predicted activity + tree-vote share
         |
         v
Bluetooth Low Energy notifications
         |
         v
Python activity recognition GUI + optional CSV logging
```

The displayed tree-vote share is the proportion of trees voting for the predicted class, **not** a calibrated probability.

## Model Training and Evaluation

IMU recordings were processed in Python to extract window-level statistical features and train a Random Forest classifier. Trial-based cross-validation was used to evaluate performance on held-out recordings.

An earlier five-activity offline leave-one-trial-out evaluation achieved **92.36% overall accuracy**. This result describes that offline evaluation and should not be interpreted as the accuracy of the final embedded 0.5-second model without evaluating that exact configuration.

The trained model was converted into a compact C++ representation for execution on the Arduino.

## Real-Time Arduino Implementation

The final Arduino firmware performs feature extraction and classification onboard, then broadcasts the predicted activity and tree-vote share using BLE notifications.

- **BLE device name:** `OpenExo-IMU`
- **Activity characteristic UUID:** `12345678-1234-1234-1234-1234567890ac`
- **Example notification:** `sitting,0.7050`

The Python GUI receives predictions; it does not run the Random Forest inference.

## Python Activity Recognition GUI

The standalone PySide6 GUI provides:

- Live predicted activity and Random Forest tree-vote share
- Connect and Disconnect controls for the Arduino BLE connection
- A ground-truth activity dropdown for labeled testing
- Automatic CSV logging

CSV recordings contain the columns:

`timestamp, actual_activity, predicted_activity, vote_share`

The GUI writes recordings to `Python_GUI/imu_results/`. Generated recordings need not all be committed to the repository; representative test results can be kept in `Results/`.

## Running the Live GUI

1. Open the final Arduino classifier sketch in the Arduino IDE, install the required Arduino libraries (`Arduino_BMI270_BMM150` and `ArduinoBLE`), and upload the firmware to the Arduino Nano 33 BLE Sense Rev2.
2. Power the Arduino and ensure BLE is available on the computer.
3. From the repository root, install the Python dependencies:

   ```bash
   python -m pip install -r requirements.txt
   ```

4. Launch the standalone GUI using the command that matches your working Python path configuration. For example, from the `Python_GUI` directory, if its package imports are configured:

   ```bash
   python pages/MachineLearningWindow.py
   ```

   **Note:** `MachineLearningWindow.py` imports `services.ArduinoBLEReader`. The `Python_GUI` directory must be on Python's module search path. If the command above fails with `ModuleNotFoundError`, use the launch method already verified in your local setup and document that exact command here.

5. Click **Connect** and wait for **Connected via BLE**.
6. Observe live predictions. For labeled testing, select the actual activity in the dropdown.
7. Click **Disconnect** when finished.

## Validation Results

Preliminary live testing of the Arduino and BLE GUI produced the following results:

| Activity | Correct predictions | Accuracy |
|---|---:|---:|
| Sitting | 25 / 25 | 100% |
| Standing | 44 / 44 | 100% |
| Walking | 41 / 45 | 91.1% |

A separate transition test produced **92 correct predictions out of 101 (approximately 91.1%)**. Some errors occurred near changes between activities.

These are limited preliminary tests, not comprehensive estimates across participants, environments, or sensor placements. Stair ascent and descent still require live validation.

## Repository Organization

| Folder | Contents |
|---|---|
| `Arduino/` | IMU data collection and embedded Random Forest classifier |
| `IMU_Analysis/` | Supporting analysis files |
| `Models/` | Saved trained models |
| `Notebooks/` | Data processing, model training, and visualization notebooks |
| `Python_GUI/` | Standalone BLE activity recognition GUI |
| `Results/` | Experimental results and evaluations |
| `Serial Classification/` | Earlier serial-based classification experiments |
| `Trial Data/` | Original labeled IMU recordings |

## Limitations and Future Work

- The dataset contains a limited number of repeated trials.
- Generalization across participants and sensor placements has not been extensively tested.
- Activity transitions can produce classification errors.
- Stair ascent and descent require additional live validation.
- The classifier does not currently control exoskeleton assistance.

Future work may include collecting more diverse training data, improving transition handling, and investigating how recognized activities could inform context-aware assistance.

## Acknowledgments

Developed as a research project involving the OpenExo hip exoskeleton platform.

## License

This repository is distributed under the MIT License; see `LICENSE`. Third-party OpenExo components remain subject to their respective licenses.
