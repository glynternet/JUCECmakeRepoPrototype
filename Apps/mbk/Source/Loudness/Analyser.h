#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "ValueShaper.h"
#include "MovingAverage.h"
#include "TailOff.h"
#include "RangeAdapter.h"
#include "AWeighting.h"

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
 *                               RangeAdapter (learns input range)
 *                                          ↓
 *                               ValueShaper (range mapping)
 *                                          ↓
 *                               MovingAverage (smoothing)
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
 * 4. **Loudness Calc**: Average magnitude across selected frequency bins
 * 5. **Range Adaptation**: RangeAdapter learns input range from content
 * 6. **Value Shaping**: Remap [observed input range] → [target output range]
 * 7. **Smoothing**: Moving average filter (configurable window, default 2 samples)
 * 8. **Decay**: TailOff prevents abrupt drops (configurable coefficient, default 0.8)
 * 9. **Final Clamp**: Output clamped to [0.0, 1.0]
 *
 * @see RangeAdapter, ValueShaper, MovingAverage, TailOff for individual stage details
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
     * @param movingAverageInitialWindow Smoothing window size (1-7, larger = smoother)
     * @param initialDecayExponent Decay coefficient [0-0.9999] (higher = slower decay)
     */
    explicit Analyser(std::function<void(float)> onLoudnessResult,
                      float processingBandIndexLow,
                      float processingBandIndexHigh,
                      float movingAverageInitialWindow,
                      float initialDecayExponent);

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

    /** Maps raw loudness to output range. Adjust inMin/inMax for sensitivity. */
    ValueShaper valueShaper {0.0F, 1.0F, 0.0F, 1.0F};

    /** Automatic range adaptation. Learns input range and maps to target output range. */
    RangeAdapter rangeAdapter {0.1F, 0.8F, 0.0F, 1.0F, 0.005F};

    /** Smoothing filter. Larger window = smoother but less responsive output. */
    MovingAverage movingAverage;

    /** Decay effect preventing abrupt drops in output value. */
    TailOff decayLength;

    /** Current weighting mode (Flat or AWeighted) */
    WeightingMode weightingMode = WeightingMode::AWeighted;

    /** Set the weighting mode for loudness calculation */
    void setWeightingMode(WeightingMode mode) { weightingMode = mode; }

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
    float calculateLoudness(float* data, int dataSize);

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
