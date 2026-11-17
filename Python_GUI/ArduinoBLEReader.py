
import asyncio

from bleak import BleakScanner, BleakClient
from PySide6 import QtCore


ACTIVITY_UUID = "12345678-1234-1234-1234-1234567890ac"
DEVICE_NAME = "OpenExo-IMU"


class ArduinoBLEReader(QtCore.QThread):
    predictionReceived = QtCore.Signal(str, float)
    connectionStatus = QtCore.Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

        self._running = False
        self._loop = None
        self._task = None

    def run(self):
        self._running = True

        try:
            asyncio.run(self._run_ble())
        except asyncio.CancelledError:
            pass
        except Exception as e:
            self.connectionStatus.emit(f"BLE error: {e}")
        finally:
            self._running = False
            self._loop = None
            self._task = None
            self.connectionStatus.emit("Disconnected")

    async def _run_ble(self):
        self._loop = asyncio.get_running_loop()
        self._task = asyncio.current_task()

        if not self._running:
            return

        await self._ble_loop()

    async def _ble_loop(self):
        self.connectionStatus.emit(
            f"Searching for {DEVICE_NAME}..."
        )

        device = await BleakScanner.find_device_by_name(
            DEVICE_NAME,
            timeout=15.0
        )

        if device is None:
            self.connectionStatus.emit("Nano not found")
            return

        if not self._running:
            return

        self.connectionStatus.emit("Connecting...")

        async with BleakClient(device, timeout=20.0) as client:
            if not self._running:
                return

            def notification_handler(sender, data):
                if not self._running:
                    return

                try:
                    message = data.decode("utf-8").strip()
                    activity, confidence_text = message.split(",", 1)
                    confidence = float(confidence_text)

                    self.predictionReceived.emit(
                        activity,
                        confidence
                    )

                except (ValueError, UnicodeDecodeError):
                    print(f"Invalid BLE message: {data!r}")

            await client.start_notify(
                ACTIVITY_UUID,
                notification_handler
            )

            self.connectionStatus.emit("Connected via BLE")

            try:
                while self._running and client.is_connected:
                    await asyncio.sleep(0.1)
            finally:
                if client.is_connected:
                    try:
                        await client.stop_notify(ACTIVITY_UUID)
                    except Exception:
                        pass

    def stop(self):
        self._running = False

        # Cancel the BLE task even if scanning or connecting.
        loop = self._loop
        task = self._task

        if loop is not None and task is not None:
            try:
                if loop.is_running():
                    loop.call_soon_threadsafe(task.cancel)
            except RuntimeError:
                pass

        # Do not wait on the GUI thread here.
        # The window manages shutdown asynchronously.
