
#ifndef IMU_FEATURE_EXTRACTION_H
#define IMU_FEATURE_EXTRACTION_H

#include <Arduino.h>
#include <math.h>

// Nine IMU channels:
// ax, ay, az, gx, gy, gz, mx, my, mz

const int IMU_CHANNELS = 9;
const int IMU_FEATURES = 27;

// Accumulators for one window
struct IMUWindow {
    double sum[IMU_CHANNELS];
    double sumSquares[IMU_CHANNELS];
    unsigned int count;
};

// Start a new window
void resetIMUWindow(IMUWindow &window) {
    window.count = 0;

    for (int i = 0; i < IMU_CHANNELS; i++) {
        window.sum[i] = 0.0;
        window.sumSquares[i] = 0.0;
    }
}

// Add one complete nine-axis sensor sample
void addIMUSample(IMUWindow &window, const float sample[9]) {
    for (int i = 0; i < IMU_CHANNELS; i++) {
        double value = sample[i];

        window.sum[i] += value;
        window.sumSquares[i] += value * value;
    }

    window.count++;
}

// Calculate the 27 features in the Python model's order
bool calculateIMUFeatures(
    const IMUWindow &window,
    float features[IMU_FEATURES]
) {
    if (window.count == 0) {
        return false;
    }

    double N = window.count;

    for (int i = 0; i < IMU_CHANNELS; i++) {

        double mean = window.sum[i] / N;

        double meanSquare = window.sumSquares[i] / N;

        // Population variance: ddof = 0
        double variance = meanSquare - mean * mean;

        // Prevent negative values from floating-point rounding
        if (variance < 0.0) {
            variance = 0.0;
        }

        double stdDev = sqrt(variance);
        double rms = sqrt(meanSquare);

        // Match Python feature order:
        // ax_mean, ax_std, ax_rms, ay_mean, ...
        features[i * 3]     = (float)mean;
        features[i * 3 + 1] = (float)stdDev;
        features[i * 3 + 2] = (float)rms;
    }

    return true;
}

#endif
