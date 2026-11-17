#include <Arduino_BMI270_BMM150.h>

// 9-AXIS IMU DATA COLLECTION
// time    ax    ay    az    gx    gy    gz    mx    my    mz
//
// Accelerometer: ~100 Hz
// Gyroscope: ~100 Hz
// Magnetometer: ~10 Hz
//
// Sample interval: 10,000 microseconds
// ============================================================

const unsigned long SAMPLE_INTERVAL_US = 10000;

unsigned long nextSampleTime = 0;

// Store  most recent magnetometer reading

float mx = 0.0;
float my = 0.0;
float mz = 0.0;

void setup() {

  Serial.begin(230400);

  while (!Serial);

  // Start IMU

  if (!IMU.begin()) {
    Serial.println("IMU FAILED");
    while (1);
  }

  // Column headers

  Serial.println("time\tax\tay\taz\tgx\tgy\tgz\tmx\tmy\tmz");

  // Set first sampling time

  nextSampleTime = micros();
}

void loop() {

  unsigned long currentTime = micros();

  // Check if it's time for the next 100 Hz sample

  if ((long)(currentTime - nextSampleTime) >= 0) {

    // Schedule next sample
    nextSampleTime += SAMPLE_INTERVAL_US;

    // Variables for accelerometer and gyroscope

    float ax, ay, az;
    float gx, gy, gz;

    // read accelerometer

    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(ax, ay, az);
    }
    else {
      return;
    }

    // read gyroscope

    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gx, gy, gz);
    }
    else {
      return;
    }

    // Update magnetometer ONLY when a new reading is available (If no new magnetometer reading is available, mx/my/mz keep their previous values.

    if (IMU.magneticFieldAvailable()) {
      IMU.readMagneticField(mx, my, mz);
    }

    // OUTPUT ONE COMPLETE 9-AXIS ROW

    Serial.print(currentTime);
    Serial.print('\t');

    // Accelerometer
    Serial.print(ax, 3);
    Serial.print('\t');

    Serial.print(ay, 3);
    Serial.print('\t');

    Serial.print(az, 3);
    Serial.print('\t');

    // Gyroscope
    Serial.print(gx, 3);
    Serial.print('\t');

    Serial.print(gy, 3);
    Serial.print('\t');

    Serial.print(gz, 3);
    Serial.print('\t');

    // Magnetometer
    Serial.print(mx, 3);
    Serial.print('\t');

    Serial.print(my, 3);
    Serial.print('\t');

    Serial.println(mz, 3);
  }
}