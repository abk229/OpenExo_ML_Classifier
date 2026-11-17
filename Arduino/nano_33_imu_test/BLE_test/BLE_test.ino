
#include <ArduinoBLE.h>

BLEService activityService(
    "12345678-1234-1234-1234-1234567890ab"
);

BLEStringCharacteristic activityCharacteristic(
    "12345678-1234-1234-1234-1234567890ac",
    BLERead | BLENotify,
    20
);

void setup() {
    Serial.begin(230400);

    // Wait briefly for Serial Monitor
    unsigned long startTime = millis();
    while (!Serial && millis() - startTime < 5000) {
        delay(10);
    }

    Serial.println("Starting BLE test...");

    if (!BLE.begin()) {
        Serial.println("BLE initialization failed!");
        while (1) {
            delay(1000);
        }
    }

    Serial.println("BLE initialized successfully!");

    BLE.setLocalName("OpenExo-IMU");
    BLE.setAdvertisedService(activityService);

    activityService.addCharacteristic(
        activityCharacteristic
    );

    BLE.addService(activityService);

    activityCharacteristic.writeValue("standing");

    BLE.advertise();

    Serial.println("BLE advertising started!");
}



void loop() {
    BLEDevice central = BLE.central();

    if (central) {
        Serial.println("BLE central connected!");

        unsigned long lastUpdate = 0;

        while (central.connected()) {
            BLE.poll();

            if (millis() - lastUpdate >= 1000) {
                lastUpdate = millis();

                activityCharacteristic.writeValue("standing");
                Serial.println("Sent activity: standing");
            }

            delay(10);
        }

        Serial.println("BLE central disconnected!");
    }

    BLE.poll();
}

