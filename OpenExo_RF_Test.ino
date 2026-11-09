
#include "random_forest_predict.h"

// Class indices:
// 0 = sitting
// 1 = stairs_down
// 2 = stairs_up
// 3 = standing
// 4 = walking

const char* classNames[] = {
  "sitting",
  "stairs_down",
  "stairs_up",
  "standing",
  "walking"
};

const char* testNames[] = {
  "sitting",
  "standing",
  "walking",
  "stairs_up",
  "stairs_down"
};

// Five recorded IMU windows, 27 features each.
// Feature order matches the trained Python model.
const float testFeatures[5][RF_NUM_FEATURES] = {
  // Sitting
  {
    -0.12462963f, 0.00775145457f, 0.124870451f,
    0.0508148148f, 0.00764991908f, 0.0513874174f,
    0.992037037f, 0.00776736516f, 0.992067445f,
    0.0654814815f, 3.4081056f, 3.40873461f,
    -0.250962963f, 1.59126194f, 1.61093046f,
    0.0158888889f, 1.08064001f, 1.08075682f,
    7.0f, 1.36082763f, 7.13104844f,
    -13.6666667f, 0.902670934f, 13.6964445f,
    7.66666667f, 2.0f, 7.92324288f
  },

  // Standing
  {
    0.976444444f, 0.00355208164f, 0.976450905f,
    0.0839259259f, 0.00640558741f, 0.0841700219f,
    -0.149037037f, 0.00967425557f, 0.149350693f,
    -0.947222222f, 4.13481102f, 4.2419208f,
    -1.33155556f, 1.71807182f, 2.17366303f,
    -0.56737037f, 0.698115142f, 0.899596514f,
    -3.81481481f, 1.44111677f, 4.07794429f,
    -37.962963f, 1.73165477f, 38.0024366f,
    -19.4444444f, 1.44871165f, 19.498338f
  },

  // Walking
  {
    0.969888889f, 0.128648915f, 0.978383871f,
    0.0925925926f, 0.0851731392f, 0.125808791f,
    0.171296296f, 0.163072539f, 0.23650597f,
    38.6555926f, 52.2670085f, 65.0084227f,
    -40.2492963f, 41.0065655f, 57.459066f,
    22.1806667f, 16.6615296f, 27.7414589f,
    -16.9259259f, 1.41227229f, 16.9847426f,
    -14.1481481f, 1.48333218f, 14.225694f,
    -17.037037f, 0.961537406f, 17.0641491f
  },

  // Stairs up
  {
    -0.602222222f, 0.252415491f, 0.652981765f,
    0.0434444444f, 0.14681617f, 0.153109136f,
    0.649925926f, 0.187171525f, 0.676340808f,
    23.6001852f, 83.1926948f, 86.4753907f,
    0.223777778f, 59.1595129f, 59.1599361f,
    -18.853037f, 16.7260618f, 25.2031376f,
    6.18518519f, 3.232211f, 6.97880389f,
    34.5925926f, 1.49713948f, 34.6249749f,
    -10.3703704f, 3.3126933f, 10.8866211f
  },

  // Stairs down
  {
    -0.882851852f, 0.251395911f, 0.917947328f,
    0.305185185f, 0.155364574f, 0.342456053f,
    0.477296296f, 0.117838815f, 0.491627644f,
    28.5826296f, 57.1745425f, 63.9210061f,
    15.2067037f, 37.1057258f, 40.1008569f,
    -19.9607778f, 23.7761901f, 31.04416f,
    6.66666667f, 0.902670934f, 6.72750022f,
    30.9259259f, 0.899626504f, 30.9390081f,
    -19.1481481f, 2.2722758f, 19.2825002f
  }
};

void setup() {
  Serial.begin(230400);
  while (!Serial);

  Serial.println("OpenExo Random Forest - Five Activity Test");
  Serial.print("Number of trees: ");
  Serial.println(RF_NUM_TREES);
  Serial.print("Number of features: ");
  Serial.println(RF_NUM_FEATURES);
  Serial.println();

  int correct = 0;

  for (int i = 0; i < 5; i++) {
    float confidence = 0.0f;

    unsigned long startTime = micros();
    uint8_t predictedClass =
        predictActivity(testFeatures[i], &confidence);
    unsigned long inferenceTime = micros() - startTime;

    Serial.print("Actual activity: ");
    Serial.println(testNames[i]);

    Serial.print("Predicted activity: ");
    Serial.println(classNames[predictedClass]);

    Serial.print("Confidence: ");
    Serial.println(confidence, 4);

    Serial.print("Inference time (microseconds): ");
    Serial.println(inferenceTime);

    if (strcmp(testNames[i], classNames[predictedClass]) == 0) {
      correct++;
      Serial.println("Result: PASS");
    } else {
      Serial.println("Result: FAIL");
    }

    Serial.println();
  }

  Serial.print("Total correct: ");
  Serial.print(correct);
  Serial.println(" / 5");
}

void loop() {
}
