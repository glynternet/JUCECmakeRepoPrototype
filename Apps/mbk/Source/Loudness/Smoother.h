#pragma once

#include <algorithm>
#include "PipelineStage.h"

namespace Loudness {

/**
 * @brief Exponentially Weighted Moving Average (EWMA) smoother for loudness values.
 *
 * Provides smooth, responsive output with O(1) computation per sample.
 * Unlike a simple moving average, EWMA gives higher weight to recent values
 * while still considering history with exponentially decaying weights.
 *
 * ## Algorithm
 *
 * ```
 * average = α * new_value + (1 - α) * previous_average
 * ```
 *
 * Where α (alpha) controls responsiveness:
 * - α = 1.0: No smoothing (output equals input)
 * - α → 0: Heavy smoothing (output changes slowly)
 *
 * ## Usage in Processing Chain
 *
 * The Smoother sits after value shaping and before decay:
 * ```
 * Shaped Value → Smoother → Smoother Value → TailOff
 * ```
 *
 * ## Trade-offs
 *
 * - **Lower smoothing (0.0-0.3)**: Faster response, more reactive to transients
 * - **Higher smoothing (0.7-1.0)**: Smoother output, more lag
 *
 * Default smoothing of 0.1 provides light smoothing while maintaining responsiveness.
 */
class Smoother : public PipelineStage {
private:
    float average = 0.0f;
    float alpha = 1.0f;  // Default: no smoothing
    bool initialized = false;

public:
    /**
     * @param smoothing Smoothing amount (0.0 = none, 1.0 = heavy)
     */
    explicit Smoother(float smoothing = 0.0f) { setSmoothing(smoothing); }

    ~Smoother() = default;

    /**
     * @brief Add a new value and return the updated smoothed average.
     *
     * First value initializes the average directly (no smoothing applied).
     * Subsequent values are blended using EWMA formula.
     *
     * @return The new smoothed average after incorporating the value
     */
    float add(float value) {
        if (!initialized) {
            average = value;
            initialized = true;
        } else {
            average = alpha * value + (1.0f - alpha) * average;
        }
        return average;
    }

    /** @brief Get the current smoothed average */
    float get() const { return average; }

    /**
     * @brief Set the smoothing amount.
     * @param smoothing 0.0 = no smoothing (instant response), 1.0 = heavy smoothing
     *
     * Internally converts to alpha: α = 1.0 - smoothing * 0.95
     * This maps smoothing [0, 1] to alpha [1.0, 0.05]
     */
    void setSmoothing(float smoothing) {
        smoothing = std::clamp(smoothing, 0.0f, 1.0f);
        alpha = 1.0f - smoothing * 0.95f;
    }

    /** @brief Get the current smoothing amount (0.0-1.0) */
    float getSmoothing() const { return (1.0f - alpha) / 0.95f; }

    // PipelineStage interface
    float process(float input) override { return add(input); }
    [[nodiscard]] const char* name() const override { return "Smooth"; }
};

}  // namespace Loudness
