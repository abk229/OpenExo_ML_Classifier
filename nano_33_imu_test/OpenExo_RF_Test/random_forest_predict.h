
#pragma once

#include <stdint.h>
#include "random_forest_compact.h"

// Activity class order:
// 0 = sitting
// 1 = stairs_down
// 2 = stairs_up
// 3 = standing
// 4 = walking

// Return predicted class index.
// Optionally return confidence as a value from 0.0 to 1.0.
inline uint8_t predictActivity(
    const float features[RF_NUM_FEATURES],
    float* confidence = nullptr
) {
    uint16_t votes[RF_NUM_CLASSES] = {0};

    for (int t = 0; t < RF_NUM_TREES; ++t) {
        const RFNode* tree = rf_trees[t];
        int nodeIndex = 0;

        while (tree[nodeIndex].feature >= 0) {
            const RFNode& node = tree[nodeIndex];

            if (features[node.feature] <= node.threshold) {
                nodeIndex = node.left;
            } else {
                nodeIndex = node.right;
            }
        }

        uint8_t classId = tree[nodeIndex].class_id;
        votes[classId]++;
    }

    uint8_t bestClass = 0;

    for (uint8_t c = 1; c < RF_NUM_CLASSES; ++c) {
        if (votes[c] > votes[bestClass]) {
            bestClass = c;
        }
    }

    if (confidence != nullptr) {
        *confidence =
            static_cast<float>(votes[bestClass]) / RF_NUM_TREES;
    }

    return bestClass;
}
