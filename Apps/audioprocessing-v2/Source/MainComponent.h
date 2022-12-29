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
 mainClass:        MainComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

#include "../../mbk/Source/AvvaOSCSender.h"
#include "JuceHeader.h"
#include "Loudness/AnalyserComponent.h"
#include "../../mbk/Source/AudioSourceComponent.h"
#include "../../mbk/Source/Logger.h"
#include "../../mbk/Source/StdoutLogger.h"
#include "../../mbk/Source/OSCComponent.h"

class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent()
    {
        addAndMakeVisible(audioSource);
        addAndMakeVisible(analyserComponent);
        addAndMakeVisible(&oscComponent);

        setSize(900, 500);
        setAudioChannels(2, 2);
    }

    ~MainComponent() override { shutdownAudio(); }

    //==============================================================================
    // Component functions

    void resized() override
    {
        auto bounds = getLocalBounds();

        auto settingsBounds = bounds.removeFromLeft(500);
        oscComponent.setBounds(settingsBounds.removeFromBottom(40));
        audioSource.setBounds(settingsBounds);
        analyserComponent.setBounds(bounds);
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
            analyserComponent.pushNextSampleIntoFifo(frameValues[i]);
        }
    }

    //==============================================================================
    void paint(Graphics& g) override { g.fillAll(Colours::black); }

    // ===============================
    // OSC functions
    // TODO(glynternet): replace with other logger
    AudioApp::OSCComponent oscComponent {logger};
    AudioApp::AvvaOSCSender sender {oscComponent};

    StdoutLogger logger {false};
    AudioApp::AudioSourceComponent audioSource {deviceManager, logger};

    Loudness::AnalyserComponent analyserComponent {sender};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
