
import serial
from PySide6 import QtCore


class ArduinoActivityReader(QtCore.QThread):

    # Signals sent to the GUI
    predictionReceived = QtCore.Signal(str, float)
    connectionStatus = QtCore.Signal(str)

    def __init__(self, port="COM3", baud_rate=230400, parent=None):
        super().__init__(parent)

        self.port = port
        self.baud_rate = baud_rate
        self._running = False
        self._serial = None

    def run(self):
        self._running = True
        activity = None

        try:
            with serial.Serial(
                self.port,
                self.baud_rate,
                timeout=0.2
            ) as arduino:

                self._serial = arduino
                self.connectionStatus.emit(
                    f"Connected to {self.port}"
                )

                while self._running:

                    line = arduino.readline().decode(
                        "utf-8", errors="replace"
                    ).strip()

                    if line.startswith("Predicted activity:"):
                        activity = line.split(":", 1)[1].strip()

                    elif (
                        line.startswith("Confidence:")
                        and activity is not None
                    ):
                        try:
                            confidence = float(
                                line.split(":", 1)[1].strip()
                            )

                            self.predictionReceived.emit(
                                activity,
                                confidence
                            )

                        except ValueError:
                            pass

                        activity = None

        except serial.SerialException as e:
            self.connectionStatus.emit(
                f"Serial error: {e}"
            )

        finally:
            self._serial = None
            self.connectionStatus.emit("Disconnected")

    def stop(self):
        self._running = False
        self.wait(2000)
