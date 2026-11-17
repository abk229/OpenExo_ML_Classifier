
#include <Arduino_BMI270_BMM150.h>
#include "imu_feature_extraction.h"
#include "random_forest_predict.h"

// ============================================================
// OpenExo - Live IMU Feature Extraction
//
// Accelerometer + gyroscope: approximately 100 Hz
// Magnetometer: approximately 10 Hz
// Feature window: 0.5 seconds
// Features: mean, std, RMS for all nine axes
// ============================================================

const unsigned long SAMPLE_INTERVAL_US = 10000;
const unsigned long MAG_INTERVAL_US = 100000;
const unsigned long WINDOW_INTERVAL_US = 500000;

const char* featureNames[27] = {
  "ax_mean", "ax_std", "ax_rms",
  "ay_mean", "ay_std", "ay_rms",
  "az_mean", "az_std", "az_rms",
  "gx_mean", "gx_std", "gx_rms",
  "gy_mean", "gy_std", "gy_rms",
  "gz_mean", "gz_std", "gz_rms",
  "mx_mean", "mx_std", "mx_rms",
  "my_mean", "my_std", "my_rms",
  "mz_mean", "mz_std", "mz_rms"
};

const char* classNames[] = {
  "sitting",
  "stairs_down",
  "stairs_up",
  "standing",
  "walking"
};

IMUWindow currentWindow;

unsigned long nextSampleTime = 0;
unsigned long nextMagTime = 0;
unsigned long windowStartTime = 0;

float mx = 0.0f;
float my = 0.0f;
float mz = 0.0f;

bool magValid = false;

unsigned long windowNumber = 0;

void setup() {
  Serial.begin(230400);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("IMU initialization failed!");
    while (1);
  }

  resetIMUWindow(currentWindow);

  unsigned long now = micros();

  nextSampleTime = now;
  nextMagTime = now;
  windowStartTime = now;

  Serial.println("OpenExo - Live IMU Feature Extraction");
  Serial.println("Window duration: 0.5 seconds");
  Serial.println("Features per window: 27");
  Serial.println();
}

void loop() {
  unsigned long now = micros();

  // Update the magnetometer independently at ~10 Hz
  if ((long)(now - nextMagTime) >= 0) {
    nextMagTime += MAG_INTERVAL_US;

    if (IMU.magneticFieldAvailable()) {
      IMU.readMagneticField(mx, my, mz);
      magValid = true;
    }
  }

  // Collect one nine-axis sample approximately every 10 ms
  if ((long)(now - nextSampleTime) >= 0) {
    nextSampleTime += SAMPLE_INTERVAL_US;

    if (IMU.accelerationAvailable() &&
        IMU.gyroscopeAvailable() &&
        magValid) {

      float ax, ay, az;
      float gx, gy, gz;

      IMU.readAcceleration(ax, ay, az);
      IMU.readGyroscope(gx, gy, gz);

      float sample[9] = {
        ax, ay, az,
        gx, gy, gz,
        mx, my, mz
      };

      addIMUSample(currentWindow, sample);
    }
  }

  // Finish a window every 0.5 seconds
  if ((unsigned long)(now - windowStartTime) >=
      WINDOW_INTERVAL_US) {

    float features[IMU_FEATURES];

    if (calculateIMUFeatures(currentWindow, features)) {
      float confidence = 0.0f;

      unsigned long startTime = micros();

      uint8_t predictedClass = predictActivity(features, &confidence);

      unsigned long inferenceTime = micros() - startTime;

      Serial.println("------------------------------");
      Serial.print("Predicted activity: ");
      Serial.println(classNames[predictedClass]);

      Serial.print("Confidence: ");
      Serial.println(confidence, 4);

      Serial.print("Inference time (microseconds): ");
      Serial.println(inferenceTime);

      Serial.println("==============================");
      Serial.print("Window: ");
      Serial.println(windowNumber);

      Serial.print("Samples: ");
      Serial.println(currentWindow.count);

      for (int i = 0; i < IMU_FEATURES; i++) {
        Serial.print(featureNames[i]);
        Serial.print(": ");
        Serial.println(features[i], 6);
      }

      Serial.println();
    }

    resetIMUWindow(currentWindow);

    windowNumber++;

    windowStartTime += WINDOW_INTERVAL_US;
  }
}
