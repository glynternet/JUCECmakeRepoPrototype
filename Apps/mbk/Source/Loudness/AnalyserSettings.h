#pragma once

#include "../Components/LabelledSlider.h"
#include "Analyser.h"

namespace Loudness {
class AnalyserSettings : public juce::Component {
public:
    explicit AnalyserSettings(Loudness::Analyser& loudnessAnalyser,
                              const double initialProcessingBandLow,
                              const double initialProcessingBandHigh,
                              const int movingAverageInitialWindow,
                              const float initialDecayExponent)
        : frequencyProcessingBand(
            "Frequency Band",
            0.0f,
            1.0f,
            initialProcessingBandLow,
            initialProcessingBandHigh,
            0.5f,
            [&loudnessAnalyser](const double low, const double high) {
                loudnessAnalyser.processingBandLow = low;
                loudnessAnalyser.processingBandHigh = high;
            })
        ,

        rangeIn("Range In",
                -0.1f,
                1.1f,
                0.1f,
                0.8f,
                0.3f,
                [&loudnessAnalyser](const double min, const double max) {
                    loudnessAnalyser.valueShaper.setInMin((float) min);
                    loudnessAnalyser.valueShaper.setInMax((float) max);
                })
        ,

        decayLength("Decay Length",
                    TailOff::minExponent,
                    TailOff::maxExponent,
                    initialDecayExponent,
                    jmap(0.90f, TailOff::minExponent, TailOff::maxExponent),
                    [&loudnessAnalyser](const double exponent) {
                        loudnessAnalyser.decayLength.setMaxDecayCoefficient(
                            (float) exponent);
                    })
        ,

        movingAverage("Window Size",
                      1.f,
                      7.f,
                      1.f,
                      movingAverageInitialWindow,
                      1.f,
                      [&loudnessAnalyser](const double value) {
                          loudnessAnalyser.movingAverage.setPeriod((int) value);
                      }) {
        addAndMakeVisible(rangeIn);
        addAndMakeVisible(frequencyProcessingBand);
        addAndMakeVisible(decayLength);
        addAndMakeVisible(movingAverage);
    }

private:
    void resized() override {
        auto bounds = getLocalBounds().reduced(10);
        if (bounds.getHeight() > 200) {
            bounds = bounds.removeFromTop(200);
        }
        frequencyProcessingBand.setBounds(bounds.removeFromTop(bounds.getHeight() / 4));
        rangeIn.setBounds(bounds.removeFromTop(bounds.getHeight() / 3));
        decayLength.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
        movingAverage.setBounds(bounds);
    }

    Components::LabelledSlider frequencyProcessingBand;
    Components::LabelledSlider rangeIn;
    Components::LabelledSlider decayLength;
    Components::LabelledSlider movingAverage;
};
} // namespace Loudness
