
import sys
import csv
import os
import math

from datetime import datetime
from PySide6 import QtCore, QtWidgets

from services.ArduinoBLEReader import ArduinoBLEReader


class MachineLearningWindow(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.setWindowTitle("OpenExo - Machine Learning")
        self.resize(480, 370)

        self.reader = None
        self.connected = False
        self._closing = False
        self._stopping = False

        # --------------------------------------------------
        # CSV logging
        # --------------------------------------------------

        base_dir = os.path.dirname(
            os.path.dirname(os.path.abspath(__file__))
        )

        log_dir = os.path.join(base_dir, "imu_results")
        os.makedirs(log_dir, exist_ok=True)

        timestamp = datetime.now().strftime(
            "%Y%m%d_%H%M%S_%f"
        )

        self.log_path = os.path.join(
            log_dir,
            f"live_predictions_{timestamp}.csv"
        )

        self.log_file = open(
            self.log_path,
            "w",
            newline="",
            encoding="utf-8"
        )

        self.csv_writer = csv.writer(self.log_file)

        self.csv_writer.writerow([
            "timestamp",
            "actual_activity",
            "predicted_activity",
            "vote_share"
        ])

        self.log_file.flush()

        # --------------------------------------------------
        # GUI layout
        # --------------------------------------------------

        layout = QtWidgets.QVBoxLayout(self)
        layout.setSpacing(12)

        title = QtWidgets.QLabel("Activity Recognition")
        title.setAlignment(QtCore.Qt.AlignCenter)
        title.setStyleSheet(
            "font-size: 20px; font-weight: bold;"
        )
        layout.addWidget(title)

        # Predicted activity
        self.activity_label = QtWidgets.QLabel("Waiting...")
        self.activity_label.setAlignment(QtCore.Qt.AlignCenter)
        self.activity_label.setStyleSheet(
            "font-size: 32px; font-weight: bold;"
        )
        layout.addWidget(self.activity_label)

        # Random Forest vote share
        self.confidence_label = QtWidgets.QLabel(
            "Random Forest vote share: --"
        )
        self.confidence_label.setAlignment(
            QtCore.Qt.AlignCenter
        )
        layout.addWidget(self.confidence_label)

        # --------------------------------------------------
        # Ground-truth activity selection
        # --------------------------------------------------

        actual_label = QtWidgets.QLabel(
            "Actual Activity (Ground Truth):"
        )
        actual_label.setAlignment(QtCore.Qt.AlignCenter)
        layout.addWidget(actual_label)

        self.actual_activity_combo = QtWidgets.QComboBox()

        self.actual_activity_combo.addItem(
            "Not Selected", None
        )
        self.actual_activity_combo.addItem(
            "Sitting", "sitting"
        )
        self.actual_activity_combo.addItem(
            "Standing", "standing"
        )
        self.actual_activity_combo.addItem(
            "Walking", "walking"
        )
        self.actual_activity_combo.addItem(
            "Stairs Up", "stairs_up"
        )
        self.actual_activity_combo.addItem(
            "Stairs Down", "stairs_down"
        )

        layout.addWidget(self.actual_activity_combo)

        # --------------------------------------------------
        # BLE connection controls
        # --------------------------------------------------

        button_layout = QtWidgets.QHBoxLayout()

        self.connect_button = QtWidgets.QPushButton(
            "Connect"
        )

        self.disconnect_button = QtWidgets.QPushButton(
            "Disconnect"
        )

        self.disconnect_button.setEnabled(False)

        button_layout.addWidget(self.connect_button)
        button_layout.addWidget(self.disconnect_button)

        layout.addLayout(button_layout)

        self.status_label = QtWidgets.QLabel(
            "Disconnected"
        )
        self.status_label.setAlignment(
            QtCore.Qt.AlignCenter
        )
        layout.addWidget(self.status_label)

        # CSV location
        self.log_label = QtWidgets.QLabel(
            f"Recording to: {os.path.basename(self.log_path)}"
        )
        self.log_label.setAlignment(QtCore.Qt.AlignCenter)
        self.log_label.setWordWrap(True)
        layout.addWidget(self.log_label)

        # Button signals
        self.connect_button.clicked.connect(
            self.connect_arduino
        )

        self.disconnect_button.clicked.connect(
            self.disconnect_arduino
        )

    # --------------------------------------------------
    # Connect to Arduino
    # --------------------------------------------------

    @QtCore.Slot()
    def connect_arduino(self):

        if self._closing or self._stopping:
            return

        if self.reader is not None:
            # Wait for the previous thread to finish.
            if self.reader.isRunning():
                return

            self.reader.deleteLater()
            self.reader = None

        self.connected = False

        self.activity_label.setText("Waiting...")
        self.confidence_label.setText(
            "Random Forest vote share: --"
        )

        self.status_label.setText("Starting BLE...")

        self.connect_button.setEnabled(False)
        self.disconnect_button.setEnabled(True)

        self.reader = ArduinoBLEReader(self)

        self.reader.predictionReceived.connect(
            self.update_prediction
        )

        self.reader.connectionStatus.connect(
            self.update_status
        )

        self.reader.finished.connect(
            self._reader_finished
        )

        self.reader.start()

    # --------------------------------------------------
    # Disconnect from Arduino
    # --------------------------------------------------

    @QtCore.Slot()
    def disconnect_arduino(self):

        if self.reader is None:
            return

        if self._stopping:
            return

        self._stopping = True
        self.connected = False

        self.status_label.setText("Disconnecting...")

        self.connect_button.setEnabled(False)
        self.disconnect_button.setEnabled(False)

        self.activity_label.setText("Waiting...")
        self.confidence_label.setText(
            "Random Forest vote share: --"
        )

        self.reader.stop()

        # Do not block the GUI while BLE shuts down.
        # The finished signal will restore the buttons.

    # --------------------------------------------------
    # Receive live predictions
    # --------------------------------------------------

    @QtCore.Slot(str, float)
    def update_prediction(self, activity, confidence):

        if not self.connected or self._closing:
            return

        self.activity_label.setText(
            activity.replace("_", " ").title()
        )

        if not math.isfinite(confidence):
            self.confidence_label.setText(
                "Random Forest vote share: --"
            )
        else:
            self.confidence_label.setText(
                f"Random Forest vote share: "
                f"{confidence * 100:.1f}%"
            )

        actual_activity = (
            self.actual_activity_combo.currentData()
        )

        timestamp = datetime.now().isoformat(
            timespec="milliseconds"
        )

        try:
            self.csv_writer.writerow([
                timestamp,
                actual_activity
                if actual_activity is not None else "",
                activity,
                confidence
            ])

            self.log_file.flush()

        except (OSError, ValueError) as e:
            self.status_label.setText(
                f"CSV logging error: {e}"
            )

    # --------------------------------------------------
    # Connection status
    # --------------------------------------------------

    @QtCore.Slot(str)
    def update_status(self, status):

        if self._closing or self._stopping:
            return

        self.status_label.setText(status)

        if status == "Connected via BLE":
            self.connected = True
            self.connect_button.setEnabled(False)
            self.disconnect_button.setEnabled(True)

        else:
            self.connected = False

            self.activity_label.setText("Waiting...")
            self.confidence_label.setText(
                "Random Forest vote share: --"
            )

    # --------------------------------------------------
    # BLE thread finished
    # --------------------------------------------------

    @QtCore.Slot()
    def _reader_finished(self):

        self.connected = False
        self._stopping = False

        self.activity_label.setText("Waiting...")
        self.confidence_label.setText(
            "Random Forest vote share: --"
        )

        if self.reader is not None:
            self.reader.deleteLater()
            self.reader = None

        if self._closing:
            # Try closing again now that BLE has stopped.
            QtCore.QTimer.singleShot(0, self.close)
            return

        self.connect_button.setEnabled(True)
        self.disconnect_button.setEnabled(False)

        # Preserve useful error messages from failed scans.
        current_status = self.status_label.text()

        if current_status in (
            "Disconnecting...",
            "Connected via BLE",
            "Starting BLE..."
        ):
            self.status_label.setText("Disconnected")

    # --------------------------------------------------
    # Close window safely
    # --------------------------------------------------

    def closeEvent(self, event):

        if self.reader is not None and self.reader.isRunning():

            self._closing = True
            self.connected = False

            self.connect_button.setEnabled(False)
            self.disconnect_button.setEnabled(False)

            self.status_label.setText(
                "Closing BLE connection..."
            )

            self.reader.stop()

            # Wait asynchronously for the reader to finish.
            event.ignore()
            return

        if not self.log_file.closed:
            try:
                self.log_file.flush()
                self.log_file.close()
            except OSError as e:
                print(f"Error closing CSV: {e}")

        event.accept()


if __name__ == "__main__":

    app = QtWidgets.QApplication(sys.argv)

    window = MachineLearningWindow()
    window.show()

    sys.exit(app.exec())
