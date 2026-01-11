#pragma once

#include <atomic>
#include <juce_core/juce_core.h>

namespace Loudness {

/**
 * @brief Generic adaptive min/max tracker with asymmetric rates.
 *
 * Tracks a value stream and learns the observed range over time.
 * Expands quickly when values exceed bounds, contracts slowly otherwise.
 *
 * ## Behavior
 *
 * - **Expansion (fast):** When values exceed current bounds, bounds expand quickly
 * - **Contraction (slow):** When values stay within bounds, bounds slowly contract
 *   toward a smoothed estimate of the signal
 *
 * ## Thread Safety
 *
 * All state is stored in atomics for lock-free cross-thread access:
 * - update() typically called from processing thread
 * - getters/setters typically called from UI thread
 */
class AdaptiveRange {
public:
    // Valid bounds for clamping during minSeparation enforcement.
    // These represent the expected input value domain (loudness 0-1).
    static constexpr float validMin = 0.0f;
    static constexpr float validMax = 1.0f;

    /**
     * @param initialRange Starting range bounds
     * @param adaptRate Smoothing coefficient [0.0001-0.1], smaller = slower
     * @param minSep Minimum separation between min and max (prevents degenerate ranges)
     */
    AdaptiveRange(juce::Range<float> initialRange,
                  float adaptRate,
                  float minSep = 0.25F)
        : observedMin(initialRange.getStart())
        , observedMax(initialRange.getEnd())
        , alpha(adaptRate)
        , smoothedSignal(initialRange.getStart() + initialRange.getLength() * 0.5F)
        , minSeparation(minSep) {}

    /**
     * @brief Update with a new sample value.
     *
     * Expands observed range when values exceed it, contracts slowly
     * toward smoothed signal when values are within range.
     *
     * @param value New sample value to incorporate
     */
    void update(float value) noexcept {
        if (!enabled.load(std::memory_order_relaxed)
            || locked.load(std::memory_order_relaxed)) {
            return;
        }

        float min = observedMin.load(std::memory_order_relaxed);
        float max = observedMax.load(std::memory_order_relaxed);
        float a = alpha.load(std::memory_order_relaxed);

        // Update smoothed signal tracker (slower than adaptation rate)
        float smoothed = smoothedSignal.load(std::memory_order_relaxed);
        float signalAlpha = a * 0.5f;
        smoothed = signalAlpha * value + (1.0f - signalAlpha) * smoothed;
        smoothedSignal.store(smoothed, std::memory_order_relaxed);

        float increaseRate = a;           // Fast rate for expanding
        float decreaseRate = a * 0.1f;    // Slow rate for contracting

        // Handle min: increase toward smoothed (fast), decrease toward signal (slow)
        if (value > min) {
            min = increaseRate * smoothed + (1.0f - increaseRate) * min;
        } else if (value < min) {
            min = decreaseRate * value + (1.0f - decreaseRate) * min;
        }

        // Handle max: increase toward signal (fast), decrease toward smoothed (slow)
        if (value > max) {
            max = increaseRate * value + (1.0f - increaseRate) * max;
        } else if (value < max) {
            max = decreaseRate * smoothed + (1.0f - decreaseRate) * max;
        }

        // Enforce minimum separation to prevent degenerate range.
        // First expand symmetrically around center, then clamp to valid bounds.
        //
        // Without clamping, centering around a low value (e.g., center=0.14) would
        // push min negative (e.g., -0.01). This caused a bug where loudness=0 mapped
        // to ~0.136 output instead of 0, because jmap(0, -0.01, 0.27, 0, 1) ≈ 0.136.
        // Clamping ensures min stays >= validMin so that input=0 maps to output=0.
        if (max - min < minSeparation) {
            float center = (max + min) * 0.5f;
            min = center - minSeparation * 0.5f;
            max = center + minSeparation * 0.5f;

            // Clamp to valid bounds, shifting the other bound to maintain separation
            if (min < validMin) {
                min = validMin;
                max = validMin + minSeparation;
            } else if (max > validMax) {
                max = validMax;
                min = validMax - minSeparation;
            }
        }

        observedMin.store(min, std::memory_order_relaxed);
        observedMax.store(max, std::memory_order_relaxed);
    }

    //==========================================================================
    // Getters

    /** @brief Get current observed minimum */
    [[nodiscard]] float getMin() const noexcept {
        return observedMin.load(std::memory_order_relaxed);
    }

    /** @brief Get current observed maximum */
    [[nodiscard]] float getMax() const noexcept {
        return observedMax.load(std::memory_order_relaxed);
    }

    /** @brief Get bounds as a Range */
    [[nodiscard]] juce::Range<float> getBounds() const noexcept {
        return {getMin(), getMax()};
    }

    /** @brief Get range width (max - min) */
    [[nodiscard]] float getWidth() const noexcept {
        return getMax() - getMin();
    }

    /** @brief Get range center ((max + min) / 2) */
    [[nodiscard]] float getCenter() const noexcept {
        return (getMax() + getMin()) * 0.5F;
    }

    //==========================================================================
    // Control methods

    /** @brief Enable/disable adaptation */
    void setEnabled(bool value) noexcept {
        enabled.store(value, std::memory_order_relaxed);
    }

    [[nodiscard]] bool isEnabled() const noexcept {
        return enabled.load(std::memory_order_relaxed);
    }

    /** @brief Lock current values (disable adaptation while preserving enabled state) */
    void setLocked(bool value) noexcept {
        locked.store(value, std::memory_order_relaxed);
    }

    [[nodiscard]] bool isLocked() const noexcept {
        return locked.load(std::memory_order_relaxed);
    }

    /** @brief Set adaptation rate [0.0001-0.1], smaller = slower */
    void setAdaptationRate(float rate) noexcept {
        alpha.store(rate, std::memory_order_relaxed);
    }

    [[nodiscard]] float getAdaptationRate() const noexcept {
        return alpha.load(std::memory_order_relaxed);
    }

    /** @brief Set bounds directly (bypasses adaptation) */
    void setBounds(juce::Range<float> range) noexcept {
        observedMin.store(range.getStart(), std::memory_order_relaxed);
        observedMax.store(range.getEnd(), std::memory_order_relaxed);
    }

private:
    std::atomic<float> observedMin;
    std::atomic<float> observedMax;
    std::atomic<float> alpha;
    std::atomic<float> smoothedSignal;
    std::atomic<bool> enabled{true};
    std::atomic<bool> locked{false};
    float minSeparation;
};

}  // namespace Loudness
