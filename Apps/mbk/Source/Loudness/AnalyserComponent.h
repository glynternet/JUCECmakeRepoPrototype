#pragma once

#include "JuceHeader.h"
#include "../Components/LabelledSlider.h"
#include "Analyser.h"
#include "AnalyserSettings.h"
#include "ValueHistoryComponent.h"
#include "../AudioSourceComponent.h"
#include "../Logger/Logger.h"
#include "../Logger/StdoutLogger.h"
#include "../OSCComponent.h"

namespace Loudness
{
class AnalyserComponent : public juce::Component
{
public:
    explicit AnalyserComponent(std::function<bool(float)> onLoudnessCallback)
        : onLoudness(std::move(onLoudnessCallback))
    {
        addAndMakeVisible(&valueHistoryComp);
        addAndMakeVisible(loudnessAnalyserSettings);
    }

    //==============================================================================
    // Component functions

    void resized() override
    {
        auto bounds = getLocalBounds();

        valueHistoryComp.setBounds(bounds);
        loudnessAnalyserSettings.setBounds(
            bounds.getProportion(juce::Rectangle(0.f, 0.f, 0.6f, 1.f)));
    }

    //==============================================================================
    // AudioAppComponent functions

    void pushNextSampleIntoFifo(float sample) noexcept
    {
        loudnessAnalyser.pushNextSampleIntoFifo(sample);
    }

    //==============================================================================
    void paint(Graphics& g) override { g.fillAll(Colours::black); }

private:
    std::function<bool(float)> onLoudness;
    float lastLevelSent = -10.f; // set to strange value to start off with

    Loudness::Analyser loudnessAnalyser {
        [this](float level)
        {
            valueHistoryComp.addLevel(level);
            if (level != lastLevelSent)
            {
                if (onLoudness && onLoudness(level))
                    lastLevelSent = level;
            }
        },
        initialProcessRateHz,
        initialProcessingBandLow,
        initialProcessingBandHigh,
        movingAverageInitialWindow,
        initialDecayExponent};
    AnalyserSettings loudnessAnalyserSettings {loudnessAnalyser,
                                               initialProcessRateHz,
                                               initialProcessingBandLow,
                                               initialProcessingBandHigh,
                                               movingAverageInitialWindow,
                                               initialDecayExponent};

    ValueHistoryComponent valueHistoryComp;

    static constexpr float initialProcessRateHz = 50.f;
    static constexpr float initialDecayExponent = 0.8f;
    static constexpr int movingAverageInitialWindow = 2;
    static constexpr double initialProcessingBandLow = 0.02;
    static constexpr double initialProcessingBandHigh = 0.13;
};
} // namespace Loudness
