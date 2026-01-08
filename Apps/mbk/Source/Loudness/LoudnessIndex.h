#pragma once

#include <atomic>
#include <cmath>
#include <limits>
#include "AdaptiveRange.h"

namespace Loudness {

/**
 * @brief Tracks long-term loudness statistics (Index and Range).
 *
 * Provides Linux load-average style metrics at three time scales:
 * - 10 seconds: Immediate responsiveness
 * - 1 minute: Short-term trend
 * - 5 minutes: Overall average
 *
 * Also tracks the dynamic range of output values.
 *
 * ## Algorithm
 *
 * Uses Exponential Weighted Moving Average (EWMA) for each time window.
 * EWMA doesn't have a hard window, but the "effective window" is approximately
 * 1/alpha samples (time for a value's influence to decay to ~37%).
 *
 * Alpha values are calculated based on the actual update rate:
 *   alpha = 1 / (windowSeconds * updatesPerSecond)
 *
 * For example, at 48kHz with 256-sample FFT (~188 updates/sec):
 * | Window | Samples | Alpha Value |
 * |--------|---------|-------------|
 * | 10 sec | 1880    | 0.000532    |
 * | 1 min  | 11280   | 0.0000887   |
 * | 5 min  | 56400   | 0.0000177   |
 *
 * ## Thread Safety
 *
 * All state is stored in atomics for lock-free cross-thread access:
 * - update() called from processing thread
 * - getters called from UI thread
 */
class LoudnessIndex {
public:
    /**
     * @param fftSize FFT size used by analyser (for calculating update rate)
     */
    explicit LoudnessIndex(int fftSize = 256)
        : fftSize_(fftSize)
        , outputRange_(0.0f, 1.0f, 0.001f, 0.05f) {
        recalculateAlphas();
    }

    /**
     * @brief Update with a new output level (call after jlimit).
     * @param level Final processed level [0.0, 1.0]
     */
    void update(float level) noexcept {
        // Update EWMA for each time window
        // EWMA: new_value = alpha * sample + (1 - alpha) * old_value
        float a10s = alpha10s_.load(std::memory_order_relaxed);
        float a1m = alpha1m_.load(std::memory_order_relaxed);
        float a5m = alpha5m_.load(std::memory_order_relaxed);

        float old10s = ema10s_.load(std::memory_order_relaxed);
        float old1m = ema1m_.load(std::memory_order_relaxed);
        float old5m = ema5m_.load(std::memory_order_relaxed);

        // Initialize to first value if uninitialized (NaN)
        ema10s_.store(std::isnan(old10s) ? level : a10s * level + (1.0f - a10s) * old10s,
                      std::memory_order_relaxed);
        ema1m_.store(std::isnan(old1m) ? level : a1m * level + (1.0f - a1m) * old1m,
                     std::memory_order_relaxed);
        ema5m_.store(std::isnan(old5m) ? level : a5m * level + (1.0f - a5m) * old5m,
                     std::memory_order_relaxed);

        // Update range tracking
        outputRange_.update(level);
    }

    /**
     * @brief Set the sample rate to recalculate alpha values.
     * @param sampleRate Audio sample rate in Hz
     */
    void setSampleRate(double sampleRate) noexcept {
        sampleRate_.store(sampleRate, std::memory_order_relaxed);
        recalculateAlphas();
    }

    //==========================================================================
    // Index values (0-10 scale)

    /** @brief Get 10-second average index (most responsive) */
    [[nodiscard]] float getIndex10s() const noexcept {
        return ema10s_.load(std::memory_order_relaxed) * 10.0f;
    }

    /** @brief Get 1-minute average index */
    [[nodiscard]] float getIndex1m() const noexcept {
        return ema1m_.load(std::memory_order_relaxed) * 10.0f;
    }

    /** @brief Get 5-minute average index (most stable) */
    [[nodiscard]] float getIndex5m() const noexcept {
        return ema5m_.load(std::memory_order_relaxed) * 10.0f;
    }

    //==========================================================================
    // Range value (0-10 scale)

    /** @brief Get observed dynamic range of output values */
    [[nodiscard]] float getRange() const noexcept {
        return outputRange_.getRange() * 10.0f;
    }

private:
    void recalculateAlphas() noexcept {
        double rate = sampleRate_.load(std::memory_order_relaxed);
        float updatesPerSec = static_cast<float>(rate / fftSize_);

        // alpha = 1 / (windowSeconds * updatesPerSecond)
        alpha10s_.store(1.0f / (window10s * updatesPerSec),
                        std::memory_order_relaxed);
        alpha1m_.store(1.0f / (window1m * updatesPerSec),
                       std::memory_order_relaxed);
        alpha5m_.store(1.0f / (window5m * updatesPerSec),
                       std::memory_order_relaxed);
    }

    // Time windows in seconds
    static constexpr float window10s = 10.0f;
    static constexpr float window1m = 60.0f;
    static constexpr float window5m = 300.0f;

    // Configuration
    int fftSize_;
    std::atomic<double> sampleRate_{44100.0};

    // Alpha values (recalculated when sample rate changes)
    std::atomic<float> alpha10s_{0.0f};
    std::atomic<float> alpha1m_{0.0f};
    std::atomic<float> alpha5m_{0.0f};

    // EWMA state for each time window (internal scale 0-1)
    // NaN indicates uninitialized - will take first incoming value
    std::atomic<float> ema10s_{std::numeric_limits<float>::quiet_NaN()};
    std::atomic<float> ema1m_{std::numeric_limits<float>::quiet_NaN()};
    std::atomic<float> ema5m_{std::numeric_limits<float>::quiet_NaN()};

    // Range tracking using AdaptiveRange
    AdaptiveRange outputRange_;
};

}  // namespace Loudness
