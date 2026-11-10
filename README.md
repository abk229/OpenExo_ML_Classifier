
#Real-Time Activity Recognition for the OpenExo Hip Exoskeleton
An embedded machine learning project that uses a wearable inertial measurement unit (IMU) to classify human activities and display live predictions in the OpenExo graphical user interface (GUI).

Project Overview
This research project explores activity recognition as a step toward context-aware assistance for a hip exoskeleton. An Arduino Nano 33 BLE Sense Rev2 collects IMU measurements, extracts features from short windows, and runs a Random Forest classifier directly on the microcontroller. Predicted activities are transmitted over USB serial and displayed in a PySide6 window integrated with the OpenExo GUI.

Current scope: Real-time activity classification and visualization. Predictions are not used to command the exoskeleton motors or change assistance settings.

Recognized Activities
Sitting
Standing
Walking
Stairs up
Stairs down
Live validation of the complete five-activity system is ongoing.

Hardware and Software
Hardware

Arduino Nano 33 BLE Sense Rev2
BMI270 accelerometer and gyroscope
BMM150 magnetometer
USB connection to a computer running the OpenExo GUI
Software

Arduino / C++ for IMU acquisition, feature extraction, and embedded inference
Python for data processing, model development, and testing
scikit-learn Random Forest for model training
PySide6 for the GUI
pyserial for communication with the Arduino
Machine Learning Pipeline
9-axis IMU measurements
          |
          v
0.5-second sensor windows
          |
          v
27 statistical features
(mean, standard deviation, RMS for each axis)
          |
          v
Embedded Random Forest classifier
          |
          v
Activity label + tree-vote share
          |
          v
USB serial (230400 baud)
          |
          v
OpenExo Machine Learning window
The sensor channels are ax, ay, az, gx, gy, gz, mx, my, and mz. The accelerometer and gyroscope operate at approximately 100 Hz, while the magnetometer updates at approximately 10 Hz; the latest magnetometer values are reused between updates.

The model uses 27 features: mean, standard deviation, and root mean square (RMS) for each of the nine axes. The displayed confidence value represents the Random Forest's tree-vote share, not a calibrated probability.

Current Implementation
Collected labeled trials for five activities
Trained a Random Forest classifier in Python
Converted the trained classifier for embedded Arduino inference
Verified agreement between Python and Arduino predictions on comparison tests
Implemented serial transmission of live predictions
Created a PySide6 activity recognition window
Added a Machine Learning Demo button to the OpenExo startup screen, allowing demonstration without a connected exoskeleton
Running the Live GUI Demo
These instructions assume the OpenExo Python GUI and Arduino firmware have already been set up.

Upload the activity recognition firmware to the Arduino Nano 33 BLE Sense Rev2.

Connect the Arduino to the computer via USB.

Close the Arduino Serial Monitor and any other programs using the serial port.

Install the required Python packages in the environment used by the OpenExo GUI:

python -m pip install PySide6 pyserial
From the Python_GUI directory, launch the OpenExo GUI:

python3 GUI.py
On Windows, python GUI.py may be used if python3 is unavailable.

Click Machine Learning Demo on the startup screen.

Observe the predicted activity and Random Forest tree-vote share update live.

Serial configuration: The current implementation uses COM3 and 230400 baud. Change the port in services/ArduinoActivityReader.py if the Arduino appears on a different port.

Relevant Project Files
The current implementation includes the following files or components:

File or component	Purpose
Python_GUI/GUI.py	OpenExo GUI entry point
Python_GUI/MainWindow.py	Main application window and Machine Learning window launch
Python_GUI/pages/ScanPage.py	Startup screen and Machine Learning Demo button
Python_GUI/pages/MachineLearningWindow.py	Live activity prediction display
Python_GUI/services/ArduinoActivityReader.py	Background serial reader using a Qt thread
OpenExo_RF_Test.ino	Arduino activity recognition sketch
random_forest_compact.h	Compact embedded Random Forest model
random_forest_predict.h	Embedded prediction logic
imu_feature_extraction.h	On-device feature extraction
File paths for the Arduino sketch and headers depend on how the firmware folder is organized in the repository.

Validation Status
The embedded classifier and live GUI have been demonstrated for basic activity changes. Structured live testing of sitting, standing, and walking is the next validation step; stair ascent and descent will be tested afterward.

Earlier offline evaluation results should not be treated as the accuracy of the current 0.5-second embedded model without evaluating that exact model and configuration.

Next Steps
Record structured live results for sitting, standing, and walking
Test stair ascent and descent in realistic conditions
Investigate prediction instability during activity transitions
Document final live validation results
Prepare the final research demonstration
Acknowledgments
Developed as a research project involving the OpenExo hip exoskeleton platform.

License
Refer to the repository's license and the applicable OpenExo project licensing terms. No license is implied by this README.

114
