#pragma once

#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "ValueShaper.h"
#include "MovingAverage.h"
#include "TailOff.h"

namespace Loudness {

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
 *                               ValueShaper (range mapping)
 *                                          ↓
 *                               MovingAverage (smoothing)
 *                                          ↓
 *                               TailOff (decay effect)
 *                                          ↓
 *                               Output [0.0 - 1.0]
 * ```
 *
 * ## Thread Model
 *
 * - **Audio thread**: Calls pushNextSampleIntoFifo() at sample rate
 * - **Timer thread**: Processes FFT at configurable rate (default 50Hz)
 * - Thread-safe communication via atomic flag (single producer/single consumer)
 *
 * ## Processing Stages
 *
 * 1. **Sample Buffering**: Audio samples accumulate in a 256-sample FIFO
 * 2. **FFT Transform**: Hann-windowed frequency-only forward FFT (256 samples)
 * 3. **Band Selection**: Extract frequency range (configurable, default 2-13% of Nyquist)
 * 4. **Loudness Calc**: Average magnitude across selected frequency bins
 * 5. **Value Shaping**: Remap raw loudness to output range (default [0.1,0.8] → [0,1])
 * 6. **Smoothing**: Moving average filter (configurable window, default 2 samples)
 * 7. **Decay**: TailOff prevents abrupt drops (configurable coefficient, default 0.8)
 *
 * @see ValueShaper, MovingAverage, TailOff for individual processing stage details
 */
class Analyser : juce::Timer {
public:
    /**
     * @brief Construct a new Analyser.
     *
     * @param onLoudnessResult Callback invoked with loudness value [0.0-1.0] each
     *                         processing cycle
     * @param processRate      FFT processing rate in Hz (default 50, range 5-70)
     * @param processingBandIndexLow  Lower frequency bound as proportion of Nyquist [0-1]
     * @param processingBandIndexHigh Upper frequency bound as proportion of Nyquist [0-1]
     * @param movingAverageInitialWindow Smoothing window size (1-7, larger = smoother)
     * @param initialDecayExponent Decay coefficient [0-0.9999] (higher = slower decay)
     */
    explicit Analyser(std::function<void(float)> onLoudnessResult,
                      float processRate,
                      float processingBandIndexLow,
                      float processingBandIndexHigh,
                      float movingAverageInitialWindow,
                      float initialDecayExponent);
    void timerCallback() override;

    /** @brief Change the FFT processing rate. @param rate New rate in Hz (5-70) */
    void setProcessRateHz(int rate);

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

    /** Smoothing filter. Larger window = smoother but less responsive output. */
    MovingAverage movingAverage;

    /** Decay effect preventing abrupt drops in output value. */
    TailOff decayLength;

private:
    /** Process FFT and invoke callback when data is ready */
    void fftTimerCallback();

    /** Calculate loudness from FFT data through the full processing chain */
    float calculateLevel();

    /** Calculate raw average loudness from FFT magnitude bins */
    float calculateLoudness(float* data, int dataSize);

    enum { fftOrder = 8, fftSize = 1 << fftOrder };

    // Atomic flag for thread-safe signaling between audio thread (pushNextSampleIntoFifo)
    // and timer thread (fftTimerCallback)
    std::atomic<bool> nextFFTBlockReady {false};
    dsp::WindowingFunction<float> window {fftSize, dsp::WindowingFunction<float>::hann};
    dsp::FFT forwardFFT {fftOrder};
    float fftData[2 * fftSize] = {};
    float fifo[fftSize] = {};
    int fifoIndex = 0;
};
} // namespace Loudness
