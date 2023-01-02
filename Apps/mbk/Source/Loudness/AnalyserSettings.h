//
// Created by glynh on 25/11/2022.
//

#ifndef JUCECMAKEREPO_LOUDNESSANALYSERSETTINGSCOMPONENT_H
#define JUCECMAKEREPO_LOUDNESSANALYSERSETTINGSCOMPONENT_H

#include "../Components/LabelledSlider.h"
#include "Analyser.h"

namespace Loudness
{
class AnalyserSettings : public juce::Component
{
public:
    explicit AnalyserSettings(Loudness::Analyser& loudnessAnalyser,
                                      const float initialProcessRateHz,
                                      const double initialProcessingBandLow,
                                      const double initialProcessingBandHigh,
                                      const int movingAverageInitialWindow,
                                      const float initialDecayExponent)
        : frequencyProcessingBand("Frequency Band",
                                  0.0f,
                                  1.0f,
                                  initialProcessingBandLow,
                                  initialProcessingBandHigh,
                                  0.5f,
                                  [&loudnessAnalyser](const double low, const double high)
                                  {
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
                [&loudnessAnalyser](const double min, const double max)
                {
                    loudnessAnalyser.valueShaper.setInMin((float) min);
                    loudnessAnalyser.valueShaper.setInMax((float) max);
                })
        ,

        decayLength("Decay Length",
                    TailOff::minExponent,
                    TailOff::maxExponent,
                    initialDecayExponent,
                    jmap(0.90f, TailOff::minExponent, TailOff::maxExponent),
                    [&loudnessAnalyser](const double exponent)
                    { loudnessAnalyser.decayLength.setExponent((float) exponent); })
        ,

        movingAverage("Window Size",
                      1.f,
                      7.f,
                      1.f,
                      movingAverageInitialWindow,
                      1.f,
                      [&loudnessAnalyser](const double value)
                      { loudnessAnalyser.movingAverage.setPeriod((int) value); })
        ,

        processRateSlider("Process rate",
                          5.f,
                          70.f,
                          initialProcessRateHz,
                          [&loudnessAnalyser](const double processRateHz)
                          { loudnessAnalyser.setProcessRateHz((int) processRateHz); })
    {
        addAndMakeVisible(rangeIn);
        addAndMakeVisible(frequencyProcessingBand);
        addAndMakeVisible(decayLength);
        addAndMakeVisible(movingAverage);
        addAndMakeVisible(processRateSlider);
    }

private:
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        if (bounds.getHeight() > 200)
        {
            bounds = bounds.removeFromTop(200);
        }
        frequencyProcessingBand.setBounds(bounds.removeFromTop(bounds.getHeight() / 5));
        rangeIn.setBounds(bounds.removeFromTop(bounds.getHeight() / 4));
        decayLength.setBounds(bounds.removeFromTop(bounds.getHeight() / 3));
        movingAverage.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
        processRateSlider.setBounds(bounds);
    }

    Components::LabelledSlider frequencyProcessingBand;
    Components::LabelledSlider rangeIn;
    Components::LabelledSlider decayLength;
    Components::LabelledSlider movingAverage;
    Components::LabelledSlider processRateSlider;
};
} // namespace Loudness

#endif //JUCECMAKEREPO_LOUDNESSANALYSERSETTINGSCOMPONENT_H
