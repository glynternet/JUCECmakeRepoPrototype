#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "ValueShaper.h"
#include "Smoother.h"
#include "TailOff.h"
#include "AdaptiveRange.h"
#include "AWeighting.h"
#include "Loudness.h"
#include "LoudnessIndex.h"

namespace Loudness {

/** Weighting mode for loudness calculation */
enum class WeightingMode {
    Flat,      ///< No frequency weighting (current behavior)
    AWeighted  ///< A-weighting for perceptual loudness
};

/**
 * @brief Real-time FFT-based loudness analyser for audio streams.
 *
 * ## Processing Chain Overview
 *
 * The Analyser receives audio samples and produces a stream of loudness values
 * through a multi-stage processing pipeline:
 *
 * ```
 * Audio Samples → FIFO Buffer → FFT → Frequency Band Selection
 *                                          ↓
 *                               Loudness Calculation
 *                                          ↓
 *                               AdaptiveRange (learns input range)
 *                                          ↓
 *                               ValueShaper (range mapping)
 *                                          ↓
 *                               Smoother (EWMA smoothing)
 *                                          ↓
 *                               TailOff (decay effect)
 *                                          ↓
 *                               Final Clamp to [0.0 - 1.0]
 * ```
 *
 * ## Thread Model
 *
 * - **Audio thread**: Calls pushNextSampleIntoFifo() at sample rate
 * - **Processing thread**: Blocks on condition variable, wakes when FIFO fills
 * - Thread-safe communication via mutex + condition_variable
 *
 * ## Processing Stages
 *
 * 1. **Sample Buffering**: Audio samples accumulate in a 256-sample FIFO
 * 2. **FFT Transform**: Hann-windowed frequency-only forward FFT (256 samples)
 * 3. **Band Selection**: Extract frequency range (configurable, default 2-13% of Nyquist)
 * 4. **Loudness Calc**: Convert magnitudes to loudness (Perceptual or Linear mode)
 * 5. **Range Adaptation**: AdaptiveRange learns input range from content
 * 6. **Value Shaping**: Remap [observed input range] → [target output range]
 * 7. **Smoothing**: EWMA filter (configurable smoothing, default 0.1)
 * 8. **Decay**: TailOff prevents abrupt drops (configurable coefficient, default 0.8)
 * 9. **Final Clamp**: Output clamped to [0.0, 1.0]
 *
 * @see AdaptiveRange, ValueShaper, Smoother, TailOff for individual stage details
 */
class Analyser {
public:
    /**
     * @brief Construct a new Analyser.
     *
     * @param onLoudnessResult Callback invoked with loudness value [0.0-1.0] each
     *                         processing cycle
     * @param processingBandIndexLow  Lower frequency bound as proportion of Nyquist [0-1]
     * @param processingBandIndexHigh Upper frequency bound as proportion of Nyquist [0-1]
     * @param initialSmoothing EWMA smoothing amount (0.0 = none, 1.0 = heavy)
     * @param initialDecayExponent Decay coefficient [0-0.9999] (higher = slower decay)
     * @param initialRangeIn Initial input range bounds
     * @param initialOutput Initial output range bounds
     * @param initialAdaptationRate Initial adaptation rate for input range
     */
    explicit Analyser(std::function<void(float)> onLoudnessResult,
                      float processingBandIndexLow,
                      float processingBandIndexHigh,
                      float initialSmoothing,
                      float initialDecayExponent,
                      juce::Range<float> initialRangeIn,
                      juce::Range<float> initialOutput,
                      float initialAdaptationRate);

    ~Analyser();

    // Non-copyable and non-movable: the class manages a thread and mutex which
    // cannot be safely copied or moved. Attempting to copy/move would either
    // require complex synchronization or leave the object in an invalid state.
    Analyser(const Analyser&) = delete;
    Analyser& operator=(const Analyser&) = delete;
    Analyser(Analyser&&) = delete;
    Analyser& operator=(Analyser&&) = delete;

    /**
     * @brief Feed an audio sample into the analyser. Called from audio thread.
     *
     * Samples accumulate in a FIFO buffer. When 256 samples are collected,
     * the buffer is copied to the FFT processing buffer and a flag signals
     * the timer thread to process.
     *
     * @param sample Audio sample value (typically -1.0 to 1.0)
     * @note Thread-safe: called from audio thread, consumed by timer thread
     */
    void pushNextSampleIntoFifo(float sample) noexcept;

    /** Callback invoked with processed loudness value [0.0-1.0] */
    std::function<void(float)> onLoudnessResult;

    /** Lower frequency bound as proportion of Nyquist (0.0-1.0) */
    double processingBandLow;

    /** Upper frequency bound as proportion of Nyquist (0.0-1.0) */
    double processingBandHigh;

    /** Maps raw loudness to output range. */
    ValueShaper valueShaper {{0.0F, 1.0F}, {0.0F, 1.0F}};

    /** Adaptive input range tracking. Learns observed min/max from audio content. */
    AdaptiveRange inputRange;

    /** Get target output range */
    [[nodiscard]] juce::Range<float> getTargetOutRange() const noexcept {
        return {targetOutMin.load(std::memory_order_relaxed),
                targetOutMax.load(std::memory_order_relaxed)};
    }

    /** Set target output range (called when user adjusts slider) */
    void setTargetOutRange(juce::Range<float> range) noexcept {
        targetOutMin.store(range.getStart(), std::memory_order_relaxed);
        targetOutMax.store(range.getEnd(), std::memory_order_relaxed);
    }

private:
    /** Target output minimum (user-configurable via UI) */
    std::atomic<float> targetOutMin;

    /** Target output maximum (user-configurable via UI) */
    std::atomic<float> targetOutMax;

public:

    /** EWMA smoothing filter. Higher smoothing = smoother but less responsive output. */
    Smoother smoother;

    /** Decay effect preventing abrupt drops in output value. */
    TailOff decayLength;

    /** Long-term loudness statistics (Index and Range at multiple time scales). */
    LoudnessIndex loudnessIndex{fftSize};

    /** Callback invoked with index values [0-10 scale] after each processing cycle.
     *  Params: (index10s, index1m, index5m, range) */
    std::function<void(float, float, float, float)> onIndexUpdate;

    /** Current weighting mode (Flat or AWeighted) */
    WeightingMode weightingMode = WeightingMode::AWeighted;

    /** Set the weighting mode for loudness calculation */
    void setWeightingMode(WeightingMode mode) { weightingMode = mode; }

    /**
     * @brief Current mapping mode (Linear or Perceptual).
     *
     * Controls how FFT magnitudes are converted to loudness values:
     * - **Linear** (default): Simple amplitude average
     * - **Perceptual**: Stevens' Power Law for true perceptual linearity
     *
     * @see MappingMode for mode descriptions
     * @see LoudnessCalculator::CalculatePerceptual() for the perceptual algorithm
     */
    MappingMode mappingMode = MappingMode::Linear;

    /**
     * @brief Set the mapping mode for loudness calculation.
     * @param mode MappingMode::Linear or MappingMode::Perceptual
     */
    void setMappingMode(MappingMode mode) { mappingMode = mode; }

    /**
     * @brief Get the current mapping mode.
     * @return Current MappingMode
     */
    [[nodiscard]] MappingMode getMappingMode() const { return mappingMode; }

    /** Set the sample rate (required for A-weighting frequency calculation) */
    void setSampleRate(double rate);

    /** FFT configuration constants - exposed for UI bin/frequency calculations */
    static constexpr int fftOrder = 8;
    static constexpr int fftSize = 1 << fftOrder; // 256
    static constexpr int maxBins = fftSize / 2; // 128

private:
    /** Main loop for the processing thread - blocks until data ready */
    void processingThreadMain();

    /** Calculate loudness from FFT data through the full processing chain */
    float calculateLevel();

    /** Calculate raw average loudness from FFT magnitude bins */
    [[nodiscard]] float calculateLoudness(float* data, int dataSize) const;

    // A-weighting support
    AWeighting aWeighting;
    double sampleRate = 44100.0;

    // Threading infrastructure for event-driven processing
    //
    // std::thread: Manages a separate OS thread that runs processingThreadMain().
    // Unlike timers, this thread can block efficiently waiting for work.
    std::thread processingThread;

    // std::mutex: A mutual exclusion lock that ensures only one thread can access
    // protected data (fftData, fftDataPending) at a time. Required for thread safety
    // when multiple threads read/write shared state.
    std::mutex mutex;

    // std::condition_variable: Allows the processing thread to sleep efficiently
    // until signaled by the audio thread. Unlike polling (checking a flag in a loop),
    // this blocks the thread with zero CPU usage until notify_one() is called.
    // Must be used with a mutex to avoid race conditions.
    std::condition_variable dataReady;

    // std::atomic<bool>: A thread-safe boolean that can be read/written from any
    // thread without a mutex. Used for the shutdown flag because it's simpler than
    // acquiring a lock just to check if we should exit.
    std::atomic<bool> shouldStop {false};

    // Protected by mutex - must hold lock before reading/writing.
    // Signals that fftData contains new samples ready for processing.
    bool fftDataPending {false};

    // FFT processing members
    dsp::WindowingFunction<float> window {fftSize, dsp::WindowingFunction<float>::hann};
    dsp::FFT forwardFFT {fftOrder};

    // Double-buffer: fifo for accumulation, fftData for processing
    float fftData[2 * fftSize] = {};
    float fifo[fftSize] = {};
    int fifoIndex = 0;
};
} // namespace Loudness
