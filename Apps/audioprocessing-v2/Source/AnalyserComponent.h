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

#include "AvvaOSCSender.h"
#include "JuceHeader.h"
#include "LabelledSlider.h"
#include "LoudnessAnalyser.h"
#include "RemoteAddressComponent.h"
#include "ValueHistoryComponent.h"
#include "../../mbk/Source/AudioSourceComponent.h"
#include "../../mbk/Source/Logger.h"
#include "../../mbk/Source/StdoutLogger.h"

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
            processingBandSlider.setVisible(visible);
            shaperInSlider.setVisible(visible);
            movingAverageSlider.setVisible(visible);
            decayLengthSlider.setVisible(visible);
            addressComponent.setVisible(visible);
            cpuUsageLabel.setVisible(visible);
            cpuUsageText.setVisible(visible);
            processRateSlider.setVisible(visible);
        };

        setSize(900, 500);

        setAudioChannels(2, 2);

        setupProcessingBandSlider(processingBandSlider);
        setupShaperInSlider(shaperInSlider);
        setupMovingAverageSlider(movingAverageSlider);
        setupDecayLengthSlider(decayLengthSlider);

        addAndMakeVisible(&addressComponent);
        addressComponent.onTextChange = [this](String text) { setRemoteAddressFromString(text); };

        addAndMakeVisible(&processRateSlider);
        Slider &slider = processRateSlider.getSlider();
        slider.setRange(5, 70);
        slider.setValue(initialProcessRateHz, dontSendNotification);
        // TODO(glynternet): do we need to stop the old timer or does doing this just change the frequency?
        slider.onValueChange = [this, &slider]() {
            loudnessAnalyser.setProcessRateHz((int) slider.getValue());
        };

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

        resizeAudioSettings(bounds.removeFromLeft(500));
        valueHistoryComp.setBounds(bounds);

        resizeSliderGroup(bounds);
        addressComponent.setBounds(getLocalBounds().removeFromBottom(20).removeFromRight(200));
    }

    void resizeSliderGroup(Rectangle<int> container) {
        constexpr int leftMargin = 10;
        constexpr int topMargin = 10;
        constexpr int height = 40;
        const int width = container.proportionOfWidth(0.65f);
        const auto leftEdge = container.getPosition().getX() + leftMargin;
        drawValueHistoryToggle.setBounds(leftEdge, topMargin, width, height);
        processingBandSlider.setBounds(leftEdge, topMargin + 1 * height, width, height);
        shaperInSlider.setBounds(leftEdge, topMargin + 2 * height, width, height);
        movingAverageSlider.setBounds(leftEdge, topMargin + 3 * height, width, height);
        decayLengthSlider.setBounds(leftEdge, topMargin + 4 * height, width, height);
        processRateSlider.setBounds(leftEdge, topMargin + 5 * height, width, height);
    }

    void resizeAudioSettings(Rectangle<int> container) {
        auto cpuSpace = container.removeFromBottom(20);
        cpuUsageLabel.setBounds(cpuSpace.removeFromLeft(100));
        cpuUsageText.setBounds(cpuSpace);
        audioSource.setBounds(container);
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

private:
    void setupProcessingBandSlider(LabelledSlider &slider) {
        addAndMakeVisible(slider);
        Slider &innerSlider = slider.getSlider();
        innerSlider.setSliderStyle(Slider::TwoValueHorizontal);
        innerSlider.setRange(0.0f, 1.0f);
        innerSlider.setSkewFactor(0.5f);  // TODO: change this from a relatively arbitrary value
        innerSlider.setMinValue(initialProcessingBandLow);
        innerSlider.setMaxValue(initialProcessingBandHigh);
        slider.onValueChange = [this, &innerSlider] {
            this->loudnessAnalyser.processingBandIndexLow = innerSlider.getMinValue();
            this->loudnessAnalyser.processingBandIndexHigh = innerSlider.getMaxValue();
        };
    }

    void setupShaperInSlider(LabelledSlider &slider) {
        addAndMakeVisible(&slider);
        auto &innerSlider = slider.getSlider();
        innerSlider.setSliderStyle(Slider::TwoValueHorizontal);
        innerSlider.setRange(-0.1f, 1.1f);
        innerSlider.setMinValue(0.1f);
        innerSlider.setMaxValue(0.8f);
        innerSlider.setSkewFactor(0.3f);  // TODO: change this from a relatively arbitrary value

        slider.onValueChange = [this, &slider]() {
            loudnessAnalyser.valueShaper.setInMin((float) slider.getSlider().getMinValue());
            loudnessAnalyser.valueShaper.setInMax((float) slider.getSlider().getMaxValue());
        };
    }

    void setupMovingAverageSlider(LabelledSlider &slider) {
        addAndMakeVisible(&slider);
        auto &innerSlider = slider.getSlider();
        innerSlider.setValue(movingAverageInitialWindow);
        innerSlider.setRange(1, 7, 1);
        slider.onValueChange = [this, &innerSlider]() {
            loudnessAnalyser.movingAverage.setPeriod((int) innerSlider.getValue());
        };
    }

    void setupDecayLengthSlider(LabelledSlider &slider) {
        addAndMakeVisible(&slider);
        auto &innerSlider = slider.getSlider();
        innerSlider.setValue(initialDecayExponent);
        innerSlider.setRange(TailOff::minExponent, TailOff::maxExponent);
        innerSlider.setSkewFactorFromMidPoint(
                jmap(0.90f, TailOff::minExponent, TailOff::maxExponent));
        slider.onValueChange = [this, &innerSlider]() {
            auto exponent = innerSlider.getValue();
            loudnessAnalyser.decayLength.setExponent((float) exponent);
        };
    }


    // ====================================================
    // MultiTimer functions
    // ====================================================

    float _lastLevelSent = -10.f; // set to strange value to start off with

    void updateCPULabel() {
        const auto cpuPercent = deviceManager.getCpuUsage() * 100.0f;
        cpuUsageText.setText(String(cpuPercent, 6) + " %", dontSendNotification);
    }

    // ===============================
    // OSC functions
    // ===============================

    void setRemoteAddressFromString(const String &text) {
        if (sender.connect(text, 9000)) {
            std::cout
                    << "AvvaOSCSender successfully connected to local socket, ready to send to "
                    << text << ":9000\n";
            return;
        }
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Connection error",
                                         "Error: could not connect to UDP port 9000.", "OK");
        addressComponent.setText("unable to connect", dontSendNotification);
        std::cout << "Unable to connect to sender at " << text << ":9000\n";
    }

    AvvaOSCSender sender;
    RemoteAddressComponent addressComponent;

    // ===============================

    LoudnessAnalyser loudnessAnalyser{[this](float level) {
        // always show level in history component
        valueHistoryComp.addLevel(level);
        // TODO(glynternet): Is it worth adding some delta checking here for is loudness is within a certain value of
        //  last then not sending it.
        if (sender.isConnected() && level != _lastLevelSent) {
            sender.sendLoudness(level);  // TODO: handle failed sends
            _lastLevelSent = level;
        }
    }, initialProcessRateHz, initialProcessingBandLow, initialProcessingBandHigh, movingAverageInitialWindow, initialDecayExponent};

    StdoutLogger logger{};
    AudioApp::AudioSourceComponent audioSource{deviceManager, logger};

    Label cpuUsageLabel{"CPU Usage", "CPU Usage"};
    Label cpuUsageText;

    LabelledSlider shaperInSlider{"Range In"};

    static constexpr float initialProcessRateHz = 50.f;
    LabelledSlider processRateSlider{"Process rate"};

    LabelledSlider decayLengthSlider{"Decay Length"};
    static constexpr float initialDecayExponent = 0.8f;

    ValueHistoryComponent valueHistoryComp;
    LabelledSlider movingAverageSlider{"Window Size"};
    static constexpr int movingAverageInitialWindow = 2;

    LabelledSlider processingBandSlider{"Frequency Band"};
    static constexpr double initialProcessingBandLow = 0.02;
    static constexpr double initialProcessingBandHigh = 0.13;

    ToggleButton drawValueHistoryToggle{"Draw Controls"};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyserComponent)
};
