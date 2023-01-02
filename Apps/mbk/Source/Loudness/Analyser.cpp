//
// Created by glynh on 24/11/2022.
//

#include "Analyser.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include "Loudness.h"

namespace Loudness
{
Analyser::Analyser(std::function<void(float)> onLoudnessResult,
                   float processRate,
                   float processingBandIndexLow,
                   float processingBandIndexHigh,
                   float movingAverageInitialWindow,
                   float initialDecayExponent)
    : movingAverage(movingAverageInitialWindow)
    , decayLength(initialDecayExponent)
{
    // TODO(glynternet): how can we fix this clang warning?
    this->onLoudnessResult = onLoudnessResult;
    this->processingBandLow = processingBandIndexLow;
    this->processingBandHigh = processingBandIndexHigh;
    // TODO(glynternet): can we remove this timer and just process everytime we receive the thing?
    //  Then we can just update the visual counterpart with a timer?
    juce::Timer::startTimerHz(processRate);
}

void Analyser::timerCallback()
{
    fftTimerCallback();
}

void Analyser::setProcessRateHz(int rate)
{
    startTimerHz(rate);
}

void Analyser::fftTimerCallback()
{
    // TODO(glynternet): Can we do that atomic bool locking thing here?
    if (!nextFFTBlockReady)
        return;
    window.multiplyWithWindowingTable(fftData, fftSize);
    forwardFFT.performFrequencyOnlyForwardTransform(fftData);
    auto level = calculateLevel();
    if (onLoudnessResult != nullptr)
    {
        onLoudnessResult(level);
    }

    nextFFTBlockReady = false;
}

// calculateLevel from the FFT data
float Analyser::calculateLevel()
{
    auto maxIndex = fftSize / 2;
    // TODO: work out a better "crossover" point as the frequency scale isn't linear
    const int indexLow = (int) ((float) maxIndex * processingBandLow);
    const int indexHigh = (int) ((float) maxIndex * processingBandHigh);

    auto level = calculateLoudness(&fftData[indexLow], indexHigh - indexLow);

    level = valueShaper.shape(level);
    movingAverage.add(level);
    level = decayLength.getValue(movingAverage.getAverage());
    return level < 0.0001f ? 0.0f : level;
}

void Analyser::pushNextSampleIntoFifo(float sample) noexcept
{
    // if the fifo contains enough data, set a flag to say
    // that the next frame should now be rendered.
    if (fifoIndex == fftSize)
    {
        // TODO(glynternet): log here if we the block hasn't been cleared since last ready.
        // TODO(glynternet): if is already ready, maybe we still want to overwrite?
        if (!nextFFTBlockReady)
        {
            // TODO(glynternet): do we need to zeromem here?
            zeromem(fftData, sizeof(fftData));
            // TODO(glynternet): is fftData always the same size as fifo and does the memcpy work as expected?
            memcpy(fftData, fifo, sizeof(fftData));
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}

// calculateLoudness will calculate the loudness for a given range of gain values of an FFT calculation
float Analyser::calculateLoudness(float* data, int dataSize)
{
    const auto mindB = -100.0f;
    const auto maxdB = 0.0f;

    /*
        why is this value calculated?!?!
        Maybe it's something to do with the maximum/minimum level that an FFT calculation
       can result in?
    */
    float fftSizeInDB = juce::Decibels::gainToDecibels((float) fftSize);

    return Loudness::Calculate(data, dataSize, mindB, maxdB, fftSizeInDB);
}
} // namespace Loudness
