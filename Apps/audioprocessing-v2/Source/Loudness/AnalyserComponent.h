#pragma once
//
// Created by glynh on 29/12/2022.

#ifndef JUCECMAKEREPO_ANALYSERCOMPONENT_H
#define JUCECMAKEREPO_ANALYSERCOMPONENT_H

#include "../../../mbk/Source/AvvaOSCSender.h"
#include "JuceHeader.h"
#include "../Components/LabelledSlider.h"
#include "Analyser.h"
#include "AnalyserSettings.h"
#include "ValueHistoryComponent.h"
#include "../../../mbk/Source/AudioSourceComponent.h"
#include "../../../mbk/Source/Logger.h"
#include "../../../mbk/Source/StdoutLogger.h"
#include "../../../mbk/Source/OSCComponent.h"

namespace Loudness
{
class AnalyserComponent : public juce::Component
{
public:
    AnalyserComponent(AudioApp::AvvaOSCSender sender)
        : sender(sender)
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

    float _lastLevelSent = -10.f; // set to strange value to start off with

    // ===============================
    // OSC functions
    AudioApp::AvvaOSCSender sender;

    Loudness::Analyser loudnessAnalyser {[this](float level)
                                         {
                                             // always show level in history component
                                             valueHistoryComp.addLevel(level);
                                             // TODO(glynternet): Is it worth adding some delta checking here for is loudness is within a certain value of
                                             //  last then not sending it.
                                             if (level != _lastLevelSent)
                                             {
                                                 sender.sendLoudness(
                                                     level); // TODO: handle failed sends
                                                 _lastLevelSent = level;
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

#endif //JUCECMAKEREPO_ANALYSERCOMPONENT_H
