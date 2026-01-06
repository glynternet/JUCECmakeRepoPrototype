#pragma once

#include "AdaptiveRange.h"
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
        : inputRange(defaultInMin, defaultInMax, adaptationRate)
        , targetOutMin(targetOutMin)
        , targetOutMax(targetOutMax) {}

    /**
     * @brief Update with a new raw loudness sample (called from processing thread).
     *
     * Expands observed range when values exceed it, contracts slowly toward
     * defaults when values are within range.
     *
     * @param rawValue Raw loudness value before any shaping
     */
    void update(float rawValue) noexcept {
        inputRange.update(rawValue);
    }

    //==========================================================================
    // Observed input range (for ValueShaper inMin/inMax)

    /** @brief Get current observed minimum input value */
    [[nodiscard]] float getObservedMin() const noexcept {
        return inputRange.getMin();
    }

    /** @brief Get current observed maximum input value */
    [[nodiscard]] float getObservedMax() const noexcept {
        return inputRange.getMax();
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
        inputRange.setEnabled(value);
    }

    [[nodiscard]] bool isEnabled() const noexcept {
        return inputRange.isEnabled();
    }

    /** @brief Lock current values (disable adaptation while preserving enabled state) */
    void setLocked(bool value) noexcept {
        inputRange.setLocked(value);
    }

    [[nodiscard]] bool isLocked() const noexcept {
        return inputRange.isLocked();
    }

    /** @brief Set adaptation rate [0.0001-0.1], smaller = slower */
    void setAdaptationRate(float rate) noexcept {
        inputRange.setAdaptationRate(rate);
    }

    [[nodiscard]] float getAdaptationRate() const noexcept {
        return inputRange.getAdaptationRate();
    }

    /** @brief Set observed range directly (e.g., from UI slider) */
    void setObservedRange(float min, float max) noexcept {
        inputRange.setBounds(min, max);
    }

private:
    // Delegates input range tracking to AdaptiveRange
    AdaptiveRange inputRange;

    // Target output range (user-configurable)
    std::atomic<float> targetOutMin;
    std::atomic<float> targetOutMax;
};

} // namespace Loudness
