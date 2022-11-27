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

#include "../../mbk/Source/AvvaOSCSender.h"
#include "JuceHeader.h"
#include "LabelledSlider.h"
#include "LoudnessAnalyser.h"
#include "LoudnessAnalyserSettings.h"
#include "ValueHistoryComponent.h"
#include "../../mbk/Source/AudioSourceComponent.h"
#include "../../mbk/Source/Logger.h"
#include "../../mbk/Source/StdoutLogger.h"
#include "../../mbk/Source/OSCComponent.h"

//==============================================================================
class AnalyserComponent : public AudioAppComponent, juce::Timer {
public:
    AnalyserComponent() {
        addAndMakeVisible(audioSource);

        cpuUsageText.setJustificationType(Justification::left);
        addAndMakeVisible(&cpuUsageLabel);
        addAndMakeVisible(&cpuUsageText);

        addAndMakeVisible(&valueHistoryComp);
        addAndMakeVisible(&drawValueHistoryToggle);
        drawValueHistoryToggle.setToggleState(true, dontSendNotification);
        drawValueHistoryToggle.onStateChange = [this]() {
            bool visible = drawValueHistoryToggle.getToggleState();
            valueHistoryComp.setVisible(visible);
            audioSource.setVisible(visible);
            cpuUsageLabel.setVisible(visible);
            cpuUsageText.setVisible(visible);
        };

        addAndMakeVisible(loudnessAnalyserSettings);

        setSize(900, 500);

        setAudioChannels(2, 2);

        addAndMakeVisible(&oscComponent);

        startTimerHz(30.f);
    }

    ~AnalyserComponent() {
        shutdownAudio();
    }

    void timerCallback() {
        updateCPULabel();
    }

    //==============================================================================
    // Component functions

    void resized() override {
        auto bounds = getLocalBounds();

        auto settingsBounds = bounds.removeFromLeft(500);
        auto cpuSpace = settingsBounds.removeFromBottom(20);
        cpuUsageLabel.setBounds(cpuSpace.removeFromLeft(100));
        cpuUsageText.setBounds(cpuSpace);
        oscComponent.setBounds(settingsBounds.removeFromBottom(40));
        audioSource.setBounds(settingsBounds);

        valueHistoryComp.setBounds(bounds);
        loudnessAnalyserSettings.setBounds(bounds.getProportion(juce::Rectangle(0.f, 0.f, 0.6f, 1.f)));
    }

    //==============================================================================
    // AudioAppComponent functions

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        audioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override { audioSource.releaseResources(); }

    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override {
        if (bufferToFill.buffer->getNumChannels() <= 0) return;
        audioSource.getNextAudioBlock(bufferToFill);
        // TODO(glynternet): This isn't great, we're copying all of the frames to another vector just to extract them.
        //   We either want to hook driectly into getNextAudioBlock or take the frame values and push all of
        //   them at the same time into the Fifo buffer? Not sure, a thing for another day.
        double *frameValues = audioSource.getFrameValues();
        for (auto i = 0; i < bufferToFill.numSamples; ++i) {
            loudnessAnalyser.pushNextSampleIntoFifo(frameValues[i]);
        }
    }

    //==============================================================================
    void paint(Graphics &g) override {
        // black background
        g.fillAll(Colours::black);
    }

    float _lastLevelSent = -10.f; // set to strange value to start off with

    void updateCPULabel() {
        const auto cpuPercent = deviceManager.getCpuUsage() * 100.0f;
        cpuUsageText.setText(String(cpuPercent, 6) + " %", dontSendNotification);
    }

    // ===============================
    // OSC functions
    AudioApp::OSCComponent oscComponent { logger };
    AudioApp::AvvaOSCSender sender { oscComponent };

    LoudnessAnalyser loudnessAnalyser{[this](float level) {
        // always show level in history component
        valueHistoryComp.addLevel(level);
        // TODO(glynternet): Is it worth adding some delta checking here for is loudness is within a certain value of
        //  last then not sending it.
        if (level != _lastLevelSent) {
            sender.sendLoudness(level);  // TODO: handle failed sends
            _lastLevelSent = level;
        }
    }, initialProcessRateHz, initialProcessingBandLow, initialProcessingBandHigh, movingAverageInitialWindow,
                                      initialDecayExponent};
    LoudnessAnalyserSettings loudnessAnalyserSettings{loudnessAnalyser, initialProcessRateHz, initialProcessingBandLow,
                                                       initialProcessingBandHigh, movingAverageInitialWindow,
                                                       initialDecayExponent};

    StdoutLogger logger { false };
    AudioApp::AudioSourceComponent audioSource{deviceManager, logger};

    Label cpuUsageLabel{"CPU usage", "CPU usage"};
    Label cpuUsageText;

    ValueHistoryComponent valueHistoryComp;

    static constexpr float initialProcessRateHz = 50.f;
    static constexpr float initialDecayExponent = 0.8f;
    static constexpr int movingAverageInitialWindow = 2;
    static constexpr double initialProcessingBandLow = 0.02;
    static constexpr double initialProcessingBandHigh = 0.13;

    ToggleButton drawValueHistoryToggle{"Draw Controls"};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyserComponent)
};
