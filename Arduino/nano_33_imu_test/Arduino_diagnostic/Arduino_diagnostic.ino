
#include <Arduino_BMI270_BMM150.h>
#include <math.h>

// ============================================================
// OpenExo - Stationary IMU Data Quality Test
//
// Continuous mode enabled
// Accelerometer and gyroscope enabled
// No BLE or machine learning
//
// Keep Arduino completely stationary.
// ============================================================

const unsigned long SAMPLE_INTERVAL_US = 10000;
const unsigned long REPORT_INTERVAL_US = 1000000;

unsigned long nextSampleTime = 0;
unsigned long reportStartTime = 0;

unsigned long sampleCount = 0;
unsigned long invalidAccelCount = 0;
unsigned long repeatedAccelCount = 0;
unsigned long repeatedGyroCount = 0;
unsigned long suddenChangeCount = 0;

float previousAx = 0;
float previousAy = 0;
float previousAz = 0;

float previousGx = 0;
float previousGy = 0;
float previousGz = 0;

bool previousValid = false;

float magnitudeSum = 0;
float magnitudeMin = 1000;
float magnitudeMax = 0;

void setup() {

  Serial.begin(230400);

  while (!Serial) {
    delay(10);
  }

  if (!IMU.begin()) {
    Serial.println("IMU initialization failed!");
    while (1);
  }

    IMU.oneShotMode();
  Serial.println("Stationary IMU Data Quality Test");
  Serial.println("Continuous mode: ENABLED");
  Serial.println("Keep the Arduino completely stationary.");
  Serial.println();

  unsigned long now = micros();

  nextSampleTime = now;
  reportStartTime = now;
}

void loop() {

  unsigned long now = micros();

  if ((long)(now - nextSampleTime) >= 0) {

    nextSampleTime += SAMPLE_INTERVAL_US;

    if (IMU.accelerationAvailable() &&
        IMU.gyroscopeAvailable()) {

      float ax, ay, az;
      float gx, gy, gz;

      IMU.readAcceleration(ax, ay, az);
      IMU.readGyroscope(gx, gy, gz);

      sampleCount++;

      // Acceleration magnitude in g
      float magnitude = sqrt(
        ax * ax +
        ay * ay +
        az * az
      );

      magnitudeSum += magnitude;

      if (magnitude < magnitudeMin) {
        magnitudeMin = magnitude;
      }

      if (magnitude > magnitudeMax) {
        magnitudeMax = magnitude;
      }

      // Near-zero acceleration is suspicious
      if (magnitude < 0.2f) {
        invalidAccelCount++;
      }

      if (previousValid) {

        // Exact repeated acceleration readings
        if (ax == previousAx &&
            ay == previousAy &&
            az == previousAz) {
          repeatedAccelCount++;
        }

        // Exact repeated gyroscope readings
        if (gx == previousGx &&
            gy == previousGy &&
            gz == previousGz) {
          repeatedGyroCount++;
        }

        // Large acceleration vector change
        float dx = ax - previousAx;
        float dy = ay - previousAy;
        float dz = az - previousAz;

        float change = sqrt(
          dx * dx +
          dy * dy +
          dz * dz
        );

        if (change > 0.5f) {
          suddenChangeCount++;
        }
      }

      previousAx = ax;
      previousAy = ay;
      previousAz = az;

      previousGx = gx;
      previousGy = gy;
      previousGz = gz;

      previousValid = true;
    }
  }

  now = micros();

  if ((unsigned long)(now - reportStartTime) >=
      REPORT_INTERVAL_US) {

    Serial.println("-----------------------------");

    Serial.print("Samples: ");
    Serial.println(sampleCount);

    Serial.print("Average magnitude (g): ");
    Serial.println(
      sampleCount > 0
        ? magnitudeSum / sampleCount
        : 0
    );

    Serial.print("Minimum magnitude (g): ");
    Serial.println(
      sampleCount > 0 ? magnitudeMin : 0
    );

    Serial.print("Maximum magnitude (g): ");
    Serial.println(magnitudeMax);

    Serial.print("Near-zero acceleration: ");
    Serial.println(invalidAccelCount);

    Serial.print("Repeated accel readings: ");
    Serial.println(repeatedAccelCount);

    Serial.print("Repeated gyro readings: ");
    Serial.println(repeatedGyroCount);

    Serial.print("Sudden accel changes: ");
    Serial.println(suddenChangeCount);

    Serial.println();

    sampleCount = 0;
    invalidAccelCount = 0;
    repeatedAccelCount = 0;
    repeatedGyroCount = 0;
    suddenChangeCount = 0;

    magnitudeSum = 0;
    magnitudeMin = 1000;
    magnitudeMax = 0;

    reportStartTime += REPORT_INTERVAL_US;
  }
}
