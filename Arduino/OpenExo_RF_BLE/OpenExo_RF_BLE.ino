
#include <Arduino_BMI270_BMM150.h>
#include <ArduinoBLE.h>

#include "imu_feature_extraction.h"
#include "random_forest_predict.h"

// ============================================================
// OpenExo - Live IMU Activity Classification
//
// ONE-SHOT MODE + SAMPLING DIAGNOSTICS
//
// Accelerometer: enabled
// Gyroscope:     enabled
// Magnetometer:  enabled
//
// Feature window: 0.5 seconds
// Features: 27 (mean, std, RMS for 9 axes)
//
// Diagnostic: Monitor sensor timing and effective sample rate.
//
// Experimental configuration.
// Do not use predictions for exoskeleton motor control.
// ============================================================

// ============================================================
// Bluetooth Low Energy (BLE)
// ============================================================

BLEService activityService(
  "12345678-1234-1234-1234-1234567890ab"
);

BLEStringCharacteristic activityCharacteristic(
  "12345678-1234-1234-1234-1234567890ac",
  BLERead | BLENotify,
  20
);

const char* activityNames[] = {
  "sitting",
  "stairs_down",
  "stairs_up",
  "standing",
  "walking"
};

// ============================================================
// Sampling intervals
// ============================================================

const unsigned long SAMPLE_INTERVAL_US = 10000;
const unsigned long MAG_INTERVAL_US = 100000;
const unsigned long WINDOW_INTERVAL_US = 500000;

// ============================================================
// IMU variables
// ============================================================

IMUWindow currentWindow;

unsigned long nextSampleTime = 0;
unsigned long nextMagTime = 0;
unsigned long windowStartTime = 0;
unsigned long windowNumber = 0;

float mx = 0.0f;
float my = 0.0f;
float mz = 0.0f;

bool magValid = false;

// ============================================================
// Sampling diagnostics
// ============================================================

unsigned long accelReadyCount = 0;
unsigned long gyroReadyCount = 0;
unsigned long bothReadyCount = 0;
unsigned long sampleAttemptCount = 0;

unsigned long lastSampleTime = 0;
unsigned long maxSampleInterval = 0;

unsigned long maxMagReadTime = 0;
unsigned long maxBLEPollTime = 0;

unsigned long maxAccelReadTime = 0;
unsigned long maxGyroReadTime = 0;

unsigned long totalAccelReadTime = 0;
unsigned long totalGyroReadTime = 0;

unsigned long accelReadCount = 0;
unsigned long gyroReadCount = 0;

// ============================================================
// Prediction stability filter
// ============================================================

const uint8_t REQUIRED_CONSECUTIVE = 2;

int stableClass = -1;
int candidateClass = -1;
uint8_t candidateCount = 0;

// ============================================================
// Setup
// ============================================================

void setup() {

  Serial.begin(230400);

  unsigned long serialStart = millis();

  while (!Serial && millis() - serialStart < 3000) {
    delay(10);
  }

  // ----------------------------------------------------------
  // Initialize IMU
  // ----------------------------------------------------------

  if (!IMU.begin()) {
    Serial.println("IMU initialization failed!");
    while (1);
  }

  // ----------------------------------------------------------
  // Use one-shot mode for consistent stationary readings
  // ----------------------------------------------------------

  IMU.oneShotMode();

  Serial.println("IMU one-shot mode enabled!");

  Serial.print("Accelerometer sample rate: ");
  Serial.println(IMU.accelerationSampleRate());

  Serial.print("Gyroscope sample rate: ");
  Serial.println(IMU.gyroscopeSampleRate());

  Serial.print("Magnetometer sample rate: ");
  Serial.println(IMU.magneticFieldSampleRate());

  // ----------------------------------------------------------
  // Initialize BLE
  // ----------------------------------------------------------

  if (!BLE.begin()) {
    Serial.println("BLE initialization failed!");
    while (1);
  }

  BLE.setLocalName("OpenExo-IMU");
  BLE.setAdvertisedService(activityService);

  activityService.addCharacteristic(activityCharacteristic);
  BLE.addService(activityService);

  // Initial placeholder value
  activityCharacteristic.writeValue("standing");

  BLE.advertise();

  Serial.println("BLE advertising started!");

  // ----------------------------------------------------------
  // Initialize feature window and timers
  // ----------------------------------------------------------

  resetIMUWindow(currentWindow);

  unsigned long now = micros();

  nextSampleTime = now;
  nextMagTime = now;
  windowStartTime = now;

  Serial.println();
  Serial.println("OpenExo - One-Shot Mode Classification");
  Serial.println("Window duration: 0.5 seconds");
  Serial.println("Features per window: 27");
  Serial.println("Accelerometer: ENABLED");
  Serial.println("Gyroscope: ENABLED");
  Serial.println("Magnetometer: ENABLED");
  Serial.println("One-shot mode: ENABLED");
  Serial.println("Stability filter: 2 consecutive predictions");
  Serial.println();
}

// ============================================================
// Main loop
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // BLE processing
  // ----------------------------------------------------------

  unsigned long bleStart = micros();

  BLE.poll();

  unsigned long bleDuration = micros() - bleStart;

  if (bleDuration > maxBLEPollTime) {
    maxBLEPollTime = bleDuration;
  }

  unsigned long now = micros();

  // ----------------------------------------------------------
  // Magnetometer sampling (~10 Hz)
  // ----------------------------------------------------------

  if ((long)(now - nextMagTime) >= 0) {

    nextMagTime += MAG_INTERVAL_US;

    if (IMU.magneticFieldAvailable()) {

      unsigned long magStart = micros();

      IMU.readMagneticField(mx, my, mz);

      unsigned long magDuration = micros() - magStart;

      if (magDuration > maxMagReadTime) {
        maxMagReadTime = magDuration;
      }

      magValid = true;
    }
  }

  // ----------------------------------------------------------
  // Accelerometer and gyroscope sampling
  // ----------------------------------------------------------

  now = micros();

  if ((long)(now - nextSampleTime) >= 0) {

    sampleAttemptCount++;

    // Measure sampling interval
    unsigned long currentSampleTime = micros();

    if (lastSampleTime != 0) {

      unsigned long interval =
          currentSampleTime - lastSampleTime;

      if (interval > maxSampleInterval) {
        maxSampleInterval = interval;
      }
    }

    lastSampleTime = currentSampleTime;

    nextSampleTime += SAMPLE_INTERVAL_US;

    // --------------------------------------------------------
    // Check sensor availability
    // --------------------------------------------------------

    bool accelReady = IMU.accelerationAvailable();
    bool gyroReady = IMU.gyroscopeAvailable();

    if (accelReady) {
      accelReadyCount++;
    }

    if (gyroReady) {
      gyroReadyCount++;
    }

    if (accelReady && gyroReady) {
      bothReadyCount++;
    }

    if (accelReady && gyroReady && magValid) {

      float ax, ay, az;
      float gx, gy, gz;

      // ------------------------------------------------------
      // Accelerometer read
      // ------------------------------------------------------

      unsigned long accelStart = micros();

      IMU.readAcceleration(ax, ay, az);

      unsigned long accelDuration =
          micros() - accelStart;

      totalAccelReadTime += accelDuration;
      accelReadCount++;

      if (accelDuration > maxAccelReadTime) {
        maxAccelReadTime = accelDuration;
      }

      // ------------------------------------------------------
      // Gyroscope read
      // ------------------------------------------------------

      unsigned long gyroStart = micros();

      IMU.readGyroscope(gx, gy, gz);

      unsigned long gyroDuration =
          micros() - gyroStart;

      totalGyroReadTime += gyroDuration;
      gyroReadCount++;

      if (gyroDuration > maxGyroReadTime) {
        maxGyroReadTime = gyroDuration;
      }

      // ------------------------------------------------------
      // Add complete nine-axis sample
      // ------------------------------------------------------

      float sample[9] = {
        ax, ay, az,
        gx, gy, gz,
        mx, my, mz
      };

      addIMUSample(currentWindow, sample);
    }
  }

  // ----------------------------------------------------------
  // Complete feature window every 0.5 seconds
  // ----------------------------------------------------------

  now = micros();

  if ((unsigned long)(now - windowStartTime) >=
      WINDOW_INTERVAL_US) {

    float features[IMU_FEATURES];

    if (calculateIMUFeatures(currentWindow, features)) {

      float confidence = 0.0f;

      // ------------------------------------------------------
      // Random Forest inference
      // ------------------------------------------------------

      unsigned long inferenceStart = micros();

      uint8_t predictedClass =
          predictActivity(features, &confidence);

      unsigned long inferenceTime =
          micros() - inferenceStart;

      // ------------------------------------------------------
      // Prediction stability filter
      // ------------------------------------------------------

      if (stableClass == -1) {

        stableClass = predictedClass;

        candidateClass = -1;
        candidateCount = 0;

      } else if (predictedClass == stableClass) {

        candidateClass = -1;
        candidateCount = 0;

      } else {

        if (predictedClass == candidateClass) {
          candidateCount++;
        } else {
          candidateClass = predictedClass;
          candidateCount = 1;
        }

        if (candidateCount >= REQUIRED_CONSECUTIVE) {

          stableClass = candidateClass;

          candidateClass = -1;
          candidateCount = 0;
        }
      }

      // ------------------------------------------------------
      // BLE transmission
      // ------------------------------------------------------

      // Preserve Python GUI compatibility.
      // BLE sends RAW prediction, not stabilized prediction.

      String bleMessage =
          String(activityNames[predictedClass])
          + ","
          + String(confidence, 4);

      activityCharacteristic.writeValue(bleMessage);

      // ------------------------------------------------------
      // Serial output
      // ------------------------------------------------------

      Serial.println("------------------------------");

      Serial.print("Raw prediction: ");
      Serial.println(activityNames[predictedClass]);

      Serial.print("Stabilized prediction: ");
      Serial.println(activityNames[stableClass]);

      Serial.print("Confidence (raw prediction): ");
      Serial.println(confidence, 4);

      Serial.print("Inference time (microseconds): ");
      Serial.println(inferenceTime);

      Serial.println("==============================");

      Serial.print("Window: ");
      Serial.println(windowNumber);

      Serial.print("Samples: ");
      Serial.println(currentWindow.count);

      Serial.print("Effective sample rate (Hz): ");
      Serial.println(currentWindow.count * 1000000.0f / WINDOW_INTERVAL_US, 1);

      Serial.print("Accel ready: ");
      Serial.println(accelReadyCount);

      Serial.print("Gyro ready: ");
      Serial.println(gyroReadyCount);

      Serial.print("Both ready: ");
      Serial.println(bothReadyCount);

      Serial.print("Sampling attempts: ");
      Serial.println(sampleAttemptCount);

      Serial.print("Max sample interval (us): ");
      Serial.println(maxSampleInterval);

      Serial.print("Max magnetometer read time (us): ");
      Serial.println(maxMagReadTime);

      Serial.print("Max BLE poll time (us): ");
      Serial.println(maxBLEPollTime);

      Serial.print("Max accel read time (us): ");
      Serial.println(maxAccelReadTime);

      Serial.print("Max gyro read time (us): ");
      Serial.println(maxGyroReadTime);

      Serial.print("Avg accel read time (us): ");
      Serial.println(
        accelReadCount > 0
          ? totalAccelReadTime / accelReadCount
          : 0
      );

      Serial.print("Avg gyro read time (us): ");
      Serial.println(
        gyroReadCount > 0
          ? totalGyroReadTime / gyroReadCount
          : 0
      );

      // ------------------------------------------------------
      // Reset diagnostics
      // ------------------------------------------------------

      accelReadyCount = 0;
      gyroReadyCount = 0;
      bothReadyCount = 0;
      sampleAttemptCount = 0;

      maxSampleInterval = 0;
      maxMagReadTime = 0;
      maxBLEPollTime = 0;

      maxAccelReadTime = 0;
      maxGyroReadTime = 0;

      totalAccelReadTime = 0;
      totalGyroReadTime = 0;

      accelReadCount = 0;
      gyroReadCount = 0;

      Serial.println();
    }

    // --------------------------------------------------------
    // Prepare next feature window
    // --------------------------------------------------------

    resetIMUWindow(currentWindow);

    windowNumber++;

    windowStartTime += WINDOW_INTERVAL_US;
  }
}
