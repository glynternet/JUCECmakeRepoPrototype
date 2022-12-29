/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2017 - ROLI Ltd.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             SpectrumAnalyserTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Displays an FFT spectrum analyser.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_dsp, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make

 type:             Component
 mainClass:        AnalyserComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

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
class AnalyserComponent : public juce::AudioAppComponent
{
public:
    AnalyserComponent()
    {
        addAndMakeVisible(audioSource);
        addAndMakeVisible(&valueHistoryComp);
        addAndMakeVisible(loudnessAnalyserSettings);
        addAndMakeVisible(&oscComponent);

        setSize(900, 500);
        setAudioChannels(2, 2);
    }

    ~AnalyserComponent() override { shutdownAudio(); }

    //==============================================================================
    // Component functions

    void resized() override
    {
        auto bounds = getLocalBounds();

        auto settingsBounds = bounds.removeFromLeft(500);
        oscComponent.setBounds(settingsBounds.removeFromBottom(40));
        audioSource.setBounds(settingsBounds);

        valueHistoryComp.setBounds(bounds);
        loudnessAnalyserSettings.setBounds(
            bounds.getProportion(juce::Rectangle(0.f, 0.f, 0.6f, 1.f)));
    }

    //==============================================================================
    // AudioAppComponent functions

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        audioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override { audioSource.releaseResources(); }

    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override
    {
        if (bufferToFill.buffer->getNumChannels() <= 0)
            return;
        audioSource.getNextAudioBlock(bufferToFill);
        // TODO(glynternet): This isn't great, we're copying all of the frames to another vector just to extract them.
        //   We either want to hook driectly into getNextAudioBlock or take the frame values and push all of
        //   them at the same time into the Fifo buffer? Not sure, a thing for another day.
        double* frameValues = audioSource.getFrameValues();
        for (auto i = 0; i < bufferToFill.numSamples; ++i)
        {
            loudnessAnalyser.pushNextSampleIntoFifo(frameValues[i]);
        }
    }

    //==============================================================================
    void paint(Graphics& g) override { g.fillAll(Colours::black); }

    float _lastLevelSent = -10.f; // set to strange value to start off with

    // ===============================
    // OSC functions
    AudioApp::OSCComponent oscComponent {logger};
    AudioApp::AvvaOSCSender sender {oscComponent};

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

    StdoutLogger logger {false};
    AudioApp::AudioSourceComponent audioSource {deviceManager, logger};

    ValueHistoryComponent valueHistoryComp;

    static constexpr float initialProcessRateHz = 50.f;
    static constexpr float initialDecayExponent = 0.8f;
    static constexpr int movingAverageInitialWindow = 2;
    static constexpr double initialProcessingBandLow = 0.02;
    static constexpr double initialProcessingBandHigh = 0.13;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyserComponent)
};
} // namespace Loudness
