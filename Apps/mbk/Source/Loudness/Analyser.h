#pragma once

#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "ValueShaper.h"
#include "MovingAverage.h"
#include "TailOff.h"

namespace Loudness
{
class Analyser : juce::Timer
{
public:
    explicit Analyser(std::function<void(float)> onLoudnessResult,
                              float processRate,
                              float processingBandIndexLow,
                              float processingBandIndexHigh,
                              float movingAverageInitialWindow,
                              float initialDecayExponent);
    void timerCallback() override;
    void setProcessRateHz(int rate);

    void pushNextSampleIntoFifo(float sample) noexcept;

    std::function<void(float)> onLoudnessResult;
    double processingBandLow;
    double processingBandHigh;
    ValueShaper valueShaper {0.0F, 1.0F, 0.0F, 1.0F};
    MovingAverage movingAverage;
    TailOff decayLength;

private:
    void fftTimerCallback();
    float calculateLevel();
    float calculateLoudness(float* data, int dataSize);

    enum
    {
        fftOrder = 8,
        fftSize = 1 << fftOrder
    };

    // Atomic flag for thread-safe signaling between audio thread (pushNextSampleIntoFifo)
    // and timer thread (fftTimerCallback)
    std::atomic<bool> nextFFTBlockReady{false};
    dsp::WindowingFunction<float> window {fftSize, dsp::WindowingFunction<float>::hann};
    dsp::FFT forwardFFT {fftOrder};
    float fftData[2 * fftSize] = {};
    float fifo[fftSize] = {};
    int fifoIndex = 0;
};
} // namespace Loudness
