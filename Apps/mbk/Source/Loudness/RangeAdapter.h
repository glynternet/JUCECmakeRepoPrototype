#pragma once

#include <atomic>

namespace Loudness {

/**
 * @brief Adaptive range tracking for automatic loudness calibration.
 *
 * Tracks raw loudness values to learn the actual input range, then maps
 * that range to a user-specified output range. This allows the user to
 * set desired output bounds (e.g., 0.3 to 0.9) and have the system
 * automatically calibrate to the audio content.
 *
 * ## Concept
 *
 * ```
 * Raw Loudness → [observedMin, observedMax] → [targetOutMin, targetOutMax]
 * ```
 *
 * - **Observed range**: Learned from actual audio content (expands/contracts automatically)
 * - **Target output range**: Set by user via UI slider
 *
 * ## Thread Safety
 * - update() called from processing thread
 * - getters/setters called from UI thread
 * - Uses atomics for lock-free cross-thread communication
 */
class RangeAdapter {
public:
    /**
     * @param defaultInMin Default/initial observed minimum
     * @param defaultInMax Default/initial observed maximum
     * @param targetOutMin Target output minimum (user-configurable)
     * @param targetOutMax Target output maximum (user-configurable)
     * @param adaptationRate Smoothing coefficient [0.0001-0.1], smaller = slower
     */
    RangeAdapter(float defaultInMin,
                 float defaultInMax,
                 float targetOutMin,
                 float targetOutMax,
                 float adaptationRate)
        : observedMin(defaultInMin)
        , observedMax(defaultInMax)
        , targetOutMin(targetOutMin)
        , targetOutMax(targetOutMax)
        , alpha(adaptationRate)
        , smoothedSignal((defaultInMin + defaultInMax) * 0.5f) {}

    /**
     * @brief Update with a new raw loudness sample (called from processing thread).
     *
     * Expands observed range when values exceed it, contracts slowly toward
     * defaults when values are within range.
     *
     * @param rawValue Raw loudness value before any shaping
     */
    void update(float rawValue) noexcept {
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
        smoothed = signalAlpha * rawValue + (1.0f - signalAlpha) * smoothed;
        smoothedSignal.store(smoothed, std::memory_order_relaxed);

        float increaseRate = a;           // Fast rate for increasing
        float decreaseRate = a * 0.1f;    // Slow rate for decreasing

        // Handle min: increase toward smoothed (fast), decrease toward signal (slow)
        if (rawValue > min) {
            min = increaseRate * smoothed + (1.0f - increaseRate) * min;
        } else if (rawValue < min) {
            min = decreaseRate * rawValue + (1.0f - decreaseRate) * min;
        }

        // Handle max: increase toward signal (fast), decrease toward smoothed (slow)
        if (rawValue > max) {
            max = increaseRate * rawValue + (1.0f - increaseRate) * max;
        } else if (rawValue < max) {
            max = decreaseRate * smoothed + (1.0f - decreaseRate) * max;
        }

        // Enforce minimum separation to prevent degenerate range
        constexpr float minSeparation = 0.05f;
        if (max - min < minSeparation) {
            float center = (max + min) * 0.5f;
            min = center - minSeparation * 0.5f;
            max = center + minSeparation * 0.5f;
        }

        observedMin.store(min, std::memory_order_relaxed);
        observedMax.store(max, std::memory_order_relaxed);
    }

    //==========================================================================
    // Observed input range (for ValueShaper inMin/inMax)

    /** @brief Get current observed minimum input value */
    [[nodiscard]] float getObservedMin() const noexcept {
        return observedMin.load(std::memory_order_relaxed);
    }

    /** @brief Get current observed maximum input value */
    [[nodiscard]] float getObservedMax() const noexcept {
        return observedMax.load(std::memory_order_relaxed);
    }

    //==========================================================================
    // Target output range (for ValueShaper outMin/outMax)

    /** @brief Get target output minimum */
    [[nodiscard]] float getTargetOutMin() const noexcept {
        return targetOutMin.load(std::memory_order_relaxed);
    }

    /** @brief Get target output maximum */
    [[nodiscard]] float getTargetOutMax() const noexcept {
        return targetOutMax.load(std::memory_order_relaxed);
    }

    /** @brief Set target output range (called when user adjusts slider) */
    void setTargetOutRange(float outMin, float outMax) noexcept {
        targetOutMin.store(outMin, std::memory_order_relaxed);
        targetOutMax.store(outMax, std::memory_order_relaxed);
    }

    //==========================================================================
    // Control methods

    /** @brief Enable/disable automatic adaptation */
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

    /** @brief Set observed range directly (e.g., from UI slider) */
    void setObservedRange(float min, float max) noexcept {
        observedMin.store(min, std::memory_order_relaxed);
        observedMax.store(max, std::memory_order_relaxed);
    }

private:
    // Observed input range (learned from actual content)
    std::atomic<float> observedMin;
    std::atomic<float> observedMax;

    // Target output range (user-configurable)
    std::atomic<float> targetOutMin;
    std::atomic<float> targetOutMax;

    std::atomic<float> alpha;
    std::atomic<bool> enabled {true};
    std::atomic<bool> locked {false};

    // Smoothed signal level for contraction targeting
    std::atomic<float> smoothedSignal;
};

} // namespace Loudness
