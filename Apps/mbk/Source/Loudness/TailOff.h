#pragma once

#include <cmath>
#include "PipelineStage.h"

namespace Loudness {

/**
 * @brief Decay/tail-off effect that prevents abrupt drops in loudness output.
 *
 * Applies a "sticky" effect where the output value decays gradually rather than
 * dropping instantly when the input decreases. This creates smoother visual
 * feedback and more natural-feeling loudness tracking.
 *
 * ## Usage in Processing Chain
 *
 * The TailOff is the final processing stage before output:
 * ```
 * MovingAverage → TailOff → Final Output [0.0-1.0]
 * ```
 *
 * ## Behavior
 *
 * For each call to getValue():
 * - If input >= previous * coefficient: output = input (immediate response to increases)
 * - If input < previous * coefficient: output = previous * coefficient (gradual decay)
 *
 * ## Coefficient Effects
 *
 * - **0.0**: No decay effect, output follows input exactly
 * - **0.5**: Fast decay, values drop quickly
 * - **0.8** (default): Moderate decay, smooth tail-off
 * - **0.95**: Slow decay, values persist longer
 * - **0.9999**: Very slow decay, values barely drop
 *
 * @note Higher coefficients may cause the output to "stick" at high values.
 *       The decay rate also depends on the processing rate (higher rate = faster decay).
 */
class TailOff : public PipelineStage {
public:
    /** @param maxDecayCoefficient Initial decay coefficient [0.0 - 0.9999] */
    explicit TailOff(float maxDecayCoefficient);

    /**
     * @brief Get the decayed output value.
     * @param in New input value
     * @return max(in, previousValue * coefficient)
     */
    float getValue(float in);

    /** @brief Set the decay coefficient. @param exp Value in range [0.0 - 0.9999] */
    void setMaxDecayCoefficient(float exp);

    static constexpr float maxExponent = 0.9999F;
    static constexpr float minExponent = 0.0F;

    // PipelineStage interface
    float process(float input) override { return getValue(input); }
    [[nodiscard]] const char* name() const override { return "Decay"; }

private:
    float exponent = 0.0F;
    float previousValue = 0.0F;
};
} // namespace Loudness
