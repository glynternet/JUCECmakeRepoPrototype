#pragma once

#include "PipelineStage.h"
#include <algorithm>

namespace Loudness {

/**
 * @brief Final pipeline stage that clamps output to [0.0, 1.0].
 *
 * Also applies a small threshold to treat near-zero values as exactly zero,
 * preventing visual noise from very small values.
 */
class Clamp : public PipelineStage {
public:
    float process(float input) override {
        if (input < threshold) {
            return 0.0F;
        }
        return std::clamp(input, 0.0F, 1.0F);
    }

    [[nodiscard]] const char* name() const override { return "Clamp (final)"; }

private:
    static constexpr float threshold = 0.0001F;
};

}  // namespace Loudness
