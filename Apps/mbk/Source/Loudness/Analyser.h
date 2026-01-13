#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Clamp.h"
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
 * - **Audio thread**: Calls pushNextSampleIntoFifo() at sample rate (~44kHz)
 * - **Processing thread**: Blocks on condition variable, wakes when frames queued
 * - **Lock-free data path**: AbstractFifo allows audio thread to queue frames
 *   without blocking, even when processing thread is busy
 * - **Condition variable**: Used only for wakeup signaling, not data protection
 *
 * The lock-free queue (AbstractFifo) decouples the audio and processing threads:
 * - Audio thread writes frames at irregular intervals (depends on buffer size)
 * - Processing thread drains all queued frames when woken
 * - Queue absorbs bursts from large audio buffers without frame drops
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
                      float initialAdaptationRate,
                      WeightingMode initialWeightingMode,
                      MappingMode mappingMode);

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
     * Samples accumulate in a local FIFO buffer. When 256 samples are collected,
     * the frame is pushed to a lock-free queue (AbstractFifo) and the processing
     * thread is signaled via condition_variable::notify_one().
     *
     * The lock-free queue allows multiple frames to be queued without blocking,
     * which is essential when large audio buffers cause many FFT frames to be
     * generated in rapid succession during a single audio callback.
     *
     * @param sample Audio sample value (typically -1.0 to 1.0)
     * @note Thread-safe: uses lock-free AbstractFifo for producer/consumer pattern
     * @note Real-time safe: never blocks (queue full = frame dropped, but rare)
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

    /** Final clamp to [0.0, 1.0] range. */
    Clamp clamp;

    /** Long-term loudness statistics (Index and Range at multiple time scales). */
    LoudnessIndex loudnessIndex{fftSize};

    /** Callback invoked with index values [0-10 scale] after each processing cycle.
     *  Params: (index10s, index1m, index5m, range) */
    std::function<void(float, float, float, float)> onIndexUpdate;

    /** Callback invoked with all pipeline stage values after each processing cycle.
     *  Values: [raw, shaped, smoothed, decayed, final] */
    std::function<void(const std::array<float, 5>&)> onPipelineUpdate;

    /** Current weighting mode (Flat or AWeighted) */
    WeightingMode weightingMode;

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
    MappingMode mappingMode;

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
    static constexpr int maxBins = fftSize / 2;   // 128

    /**
     * @brief Returns true if FFT processing cannot keep up with audio input rate.
     *
     * Persistent overload is detected when the frame queue stays above 75% full
     * for 10 or more consecutive frame writes. This distinguishes between:
     * - **Intermittent bursts**: Normal behavior with large audio buffers where
     *   many frames arrive at once but the queue drains before the next burst.
     * - **Persistent overload**: CPU cannot process FFT fast enough; frames will
     *   eventually be dropped if the condition continues.
     */
    [[nodiscard]] bool isOverloaded() const noexcept {
        return persistentOverload.load(std::memory_order_relaxed);
    }

    /** Optional callback invoked when overload state changes (true = overloaded) */
    std::function<void(bool)> onOverloadStateChanged;

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

    // =========================================================================
    // Threading Infrastructure
    // =========================================================================
    //
    // This class uses a dedicated processing thread to offload FFT computation
    // from the real-time audio thread. The key challenge is that audio callbacks
    // may deliver samples in large bursts (e.g., 4096 samples = 16 FFT frames),
    // but the processing thread can only handle one frame at a time.
    //
    // Solution: A lock-free queue (AbstractFifo) buffers frames between threads.
    // The audio thread writes frames without blocking; the processing thread
    // drains the queue when woken. This decouples the two threads' timing.

    // std::thread: Manages a dedicated OS thread running processingThreadMain().
    // Created in constructor, joined in destructor. The thread blocks efficiently
    // on the condition variable when no work is available.
    std::thread processingThread;

    // std::mutex + std::condition_variable: Used for thread wakeup signaling.
    // The mutex protects the condition_variable wait predicate, NOT the data.
    // Data transfer uses the lock-free AbstractFifo instead.
    //
    // Pattern: Audio thread calls notify_one() after queueing a frame.
    // Processing thread blocks in wait() until frames are available.
    std::mutex mutex;
    std::condition_variable dataReady;

    // std::atomic<bool>: Thread-safe shutdown flag. Atomic allows the processing
    // thread to check this without holding the mutex during normal operation.
    // Set to true in destructor, then notify_one() wakes the thread to exit.
    std::atomic<bool> shouldStop{false};

    // =========================================================================
    // Lock-Free Frame Queue (SPSC Pattern)
    // =========================================================================
    //
    // juce::AbstractFifo: A lock-free single-producer single-consumer (SPSC)
    // circular buffer. It tracks read/write positions using atomic integers,
    // allowing the audio thread (producer) and processing thread (consumer)
    // to operate concurrently without locks.
    //
    // Why lock-free? The audio thread must never block - any delay causes
    // audible glitches. With a traditional mutex, the audio thread would block
    // if the processing thread held the lock during FFT computation (~1ms).
    //
    // Capacity: 32 frames handles worst-case bursts. At 48kHz with 4096-sample
    // buffers, each audio callback generates 16 frames. Queue can absorb 2
    // full callbacks before any risk of frame drops.
    static constexpr int frameQueueCapacity = 32;
    juce::AbstractFifo frameQueueFifo{frameQueueCapacity};
    std::array<std::array<float, fftSize>, frameQueueCapacity> frameQueue;

    // =========================================================================
    // Overload Detection
    // =========================================================================
    //
    // Distinguishes between normal burst behavior and genuine CPU overload:
    // - Intermittent high fill: Large audio buffers cause bursts, but queue
    //   drains before next callback. This is normal operation.
    // - Persistent overload: Queue stays full because FFT processing is slower
    //   than the audio sample rate. This indicates a problem.
    //
    // All atomics use memory_order_relaxed because:
    // 1. We only need eventual consistency, not strict ordering
    // 2. The overload flag is informational, not used for synchronization
    // 3. Relaxed ordering has minimal performance impact on the audio thread
    std::atomic<int> consecutiveHighFill{0};
    static constexpr int highFillThreshold = 24;    // 75% of capacity
    static constexpr int overloadTriggerCount = 10; // consecutive high-fill writes
    std::atomic<bool> persistentOverload{false};

    // FFT processing members
    juce::dsp::WindowingFunction<float> window{fftSize,
                                               juce::dsp::WindowingFunction<float>::hann};
    juce::dsp::FFT forwardFFT{fftOrder};

    // Sample accumulation buffer (audio thread writes here)
    float fifo[fftSize] = {};
    int fifoIndex = 0;

    // FFT processing buffer (processing thread only)
    float fftData[2 * fftSize] = {};
};
} // namespace Loudness
