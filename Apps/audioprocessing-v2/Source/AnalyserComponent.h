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
#include "Loudness.h"
#include "MovingAverage.h"
#include "RemoteAddressComponent.h"
#include "TailOff.h"
#include "ValueHistoryComponent.h"
#include "ValueShaper.h"
#include "../../mbk/Source/AudioSourceComponent.h"
#include "../../mbk/Source/Logger.h"
#include "StdoutLogger.h"

//==============================================================================
class AnalyserComponent : public AudioAppComponent, private MultiTimer {
   public:
    StdoutLogger logger {};
    AnalyserComponent()
        : forwardFFT(fftOrder),
          window(fftSize, dsp::WindowingFunction<float>::hann),
          audioSource(deviceManager, (AudioApp::Logger &)logger),
          processingBandSlider("Frequency Band"),
          shaperInSlider("Range In"),
          valueShaper(0.0f, 1.0f, 0.0f, 1.0f),
          movingAverageSlider("Window Size"),
          movingAverage(movingAverageInitialWindow),
          decayLengthSlider("Decay Length"),
          decayLength(initialDecayExponent),
          drawValueHistoryToggle("Draw Controls")
    {
        addAndMakeVisible(audioSource);

        addAndMakeVisible(diagnosticsBox);
        diagnosticsBox.setMultiLine(true);
        diagnosticsBox.setReturnKeyStartsNewLine(true);
        diagnosticsBox.setReadOnly(true);
        diagnosticsBox.setScrollbarsShown(true);
        diagnosticsBox.setCaretVisible(false);
        diagnosticsBox.setPopupMenuEnabled(true);
        diagnosticsBox.setColour(TextEditor::backgroundColourId, Colour(0x32ffffff));
        diagnosticsBox.setColour(TextEditor::outlineColourId, Colour(0x1c000000));
        diagnosticsBox.setColour(TextEditor::shadowColourId, Colour(0x16000000));

        cpuUsageLabel.setText("CPU Usage", dontSendNotification);
        cpuUsageText.setJustificationType(Justification::right);
        addAndMakeVisible(&cpuUsageLabel);
        addAndMakeVisible(&cpuUsageText);

        addAndMakeVisible(&valueHistoryComp);
        addAndMakeVisible(&drawValueHistoryToggle);
        drawValueHistoryToggle.setToggleState(true,dontSendNotification);
        drawValueHistoryToggle.onStateChange = [this](){
            bool visible = drawValueHistoryToggle.getToggleState();
            valueHistoryComp.setVisible(visible);
            audioSource.setVisible(visible);
            diagnosticsBox.setVisible(visible);
            processingBandSlider.setVisible(visible);
            shaperInSlider.setVisible(visible);
            movingAverageSlider.setVisible(visible);
            decayLengthSlider.setVisible(visible);
            remoteAddressComp.setVisible(visible);
            cpuUsageLabel.setVisible(visible);
            cpuUsageText.setVisible(visible);
            timerFrequencySlider.setVisible(visible);
        };

        setSize(700, 500);

        setAudioChannels(2, 2);

        setupProcessingBandSlider(processingBandSlider);
        setupShaperInSlider(shaperInSlider);
        setupMovingAverageSlider(movingAverageSlider);
        setupDecayLengthSlider(decayLengthSlider);

        addAndMakeVisible(&remoteAddressComp);
        remoteAddressComp.onTextChange = [this](String text){setRemoteAddressFromString(text);};

        addAndMakeVisible(&timerFrequencySlider);
        constexpr float initialTimerHz = 30.f;
        Slider &slider = timerFrequencySlider.getSlider();
        slider.setRange(5, 60);
        slider.setValue(initialTimerHz,dontSendNotification);
        // TODO(glynternet): do we need to stop the old timer or does doing this just change the frequency?
        slider.onValueChange = [this, &slider](){
            startTimerHz(fftTimerID, (float)slider.getValue());
        };
        startTimerHz(fftTimerID, initialTimerHz);
    }

    ~AnalyserComponent() {
        shutdownAudio();
    }

    //==============================================================================
    // Component functions

    void resized() override {
        auto bounds = getLocalBounds();

        auto valueHistoryRect = bounds.removeFromTop(proportionOfHeight(0.5f));
        valueHistoryComp.setBounds(valueHistoryRect);

        resizeAudioSettings(bounds);
        resizeSliderGroup();
        remoteAddressComp.setBounds(getLocalBounds().removeFromBottom(20).removeFromRight(200));
    }

    void resizeSliderGroup() {
        constexpr int leftMargin = 10;
        constexpr int topMargin = 10;
        constexpr int height = 40;
        const int rightEdge = proportionOfWidth(1.0f - 0.35f);
        int width = rightEdge - leftMargin;
        drawValueHistoryToggle.setBounds(leftMargin, topMargin, width, height);
        processingBandSlider.setBounds(leftMargin, topMargin + 1 * height, width, height);
        shaperInSlider.setBounds(leftMargin, topMargin + 2 * height, width, height);
        movingAverageSlider.setBounds(leftMargin, topMargin + 3 * height, width, height);
        decayLengthSlider.setBounds(leftMargin, topMargin + 4 * height, width, height);
        timerFrequencySlider.setBounds(leftMargin, topMargin + 5 * height, width, height);
    }

    void resizeAudioSettings(Rectangle<int> container) {
        // get left portion of rectangle for audio device selector
        // this also removes that section from the rect
        Rectangle<int> adscBounds = container.removeFromLeft(container.proportionOfWidth(0.6f));
        audioSource.setBounds(adscBounds);

        // reduce by given amount, creating a border kinda
        container.reduce(10, 10);

        // create space for cpi usage label
        auto topLine(container.removeFromTop(20));
        cpuUsageLabel.setBounds(topLine.removeFromLeft(topLine.getWidth() / 2));
        cpuUsageText.setBounds(topLine);
        container.removeFromTop(20);

        diagnosticsBox.setBounds(container);
    }

    //==============================================================================
    // AudioAppComponent functions

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        audioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override { audioSource.releaseResources(); }

    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override {
        if (bufferToFill.buffer->getNumChannels() <= 0) return;
        audioSource.getNextAudioBlock(bufferToFill);
        // TODO(glynternet): This isn't great, we're copying all of the frames to another vector just to extract them.
        //   We either want to hook driectly into getNextAudioBlock or take the frame values and push all of
        //   them at the same time into the Fifo buffer? Not sure, a thing for another day.
        double *frameValues = audioSource.getFrameValues();
        for (auto i = 0; i < bufferToFill.numSamples; ++i) {
            pushNextSampleIntoFifo(frameValues[i]);
        }
    }

    //==============================================================================
    void paint(Graphics& g) override {
        // black background
        g.fillAll(Colours::black);
    }

    //==========================

    void pushNextSampleIntoFifo(float sample) noexcept {
        // if the fifo contains enough data, set a flag to say
        // that the next frame should now be rendered..
        if (fifoIndex == fftSize) {
            // TODO(glynternet): log here if we the block hasn't been cleared since last ready.
            // TODO(glynternet): if is already ready, maybe we still want to overwrite?
            if (!nextFFTBlockReady) {
                // TODO(glynternet): do we need to zeromem here?
                zeromem(fftData, sizeof(fftData));
                memcpy(fftData, fifo, sizeof(fftData));
                nextFFTBlockReady = true;
            }
            fifoIndex = 0;
        }
        fifo[fifoIndex++] = sample;
    }

    // calculateLevel from the FFT data
    float calculateLevel() {
        auto maxIndex = fftSize / 2;
        // TODO: work out a better "crossover" point as the frequency scale isn't linear
        const int indexLow = (int)((float)maxIndex * processingBandIndexLow);
        const int indexHigh = (int)((float)maxIndex * processingBandIndexHigh);

        auto level = calculateLoudness(&fftData[indexLow], indexHigh - indexLow);

        level = valueShaper.shape(level);
        movingAverage.add(level);
        level = decayLength.getValue(movingAverage.getAverage());
        return level < 0.0001f ? 0.0f : level;
    }

    // calculateLoudness will calculate the loudness for a given range of gain values of an FFT calculation
    float calculateLoudness(float* data, int dataSize) {
        const auto mindB = -100.0f;
        const auto maxdB = 0.0f;

        /*
            why is this value calculated?!?!
            Maybe it's something to do with the maximum/minimum level that an FFT calculation
           can result in?
        */
        float fftSizeInDB = Decibels::gainToDecibels((float)fftSize);

        return Loudness::Calculate(data, dataSize, mindB, maxdB, fftSizeInDB);
    }


   private:
    // ====================================================
    // GUI
    // ====================================================

    void setupProcessingBandSlider(LabelledSlider& slider) {
        addAndMakeVisible(slider);
        Slider& innerSlider = slider.getSlider();
        innerSlider.setSliderStyle(Slider::TwoValueHorizontal);
        innerSlider.setRange(0.0f, 1.0f);
        innerSlider.setSkewFactor(0.5f);  // TODO: change this from a relatively arbitrary value
        innerSlider.setMinValue(initialProcessingBandLow);
        innerSlider.setMaxValue(initialProcessingBandHigh);
        slider.onValueChange = [this, &innerSlider] {
            processingBandIndexLow = innerSlider.getMinValue();
            processingBandIndexHigh = innerSlider.getMaxValue();
        };
    }

    void setupShaperInSlider(LabelledSlider& slider) {
        addAndMakeVisible(&slider);
        auto& innerSlider = slider.getSlider();
        innerSlider.setSliderStyle(Slider::TwoValueHorizontal);
        innerSlider.setRange(-0.1f, 1.1f);
        innerSlider.setMinValue(0.0f);
        innerSlider.setMaxValue(1.0f);
        innerSlider.setSkewFactor(0.3f);  // TODO: change this from a relatively arbitrary value
        
        slider.onValueChange = [this, &slider]() {
            auto min = slider.getSlider().getMinValue();
            auto max = slider.getSlider().getMaxValue();
            valueShaper.setInMin((float)min);
            valueShaper.setInMax((float)max);
        };
    }

    void setupMovingAverageSlider(LabelledSlider& slider) {
        addAndMakeVisible(&slider);
        auto& innerSlider = slider.getSlider();
        innerSlider.setValue(movingAverageInitialWindow);
        innerSlider.setRange(1, 7, 1);
        slider.onValueChange = [this, &innerSlider]() {
            movingAverage.setPeriod((int)innerSlider.getValue());
        };
    }

    void setupDecayLengthSlider(LabelledSlider& slider) {
        addAndMakeVisible(&slider);
        auto& innerSlider = slider.getSlider();
        innerSlider.setValue(initialDecayExponent);
        innerSlider.setRange(TailOff::minExponent, TailOff::maxExponent);
        innerSlider.setSkewFactorFromMidPoint(
            jmap(0.90f, TailOff::minExponent, TailOff::maxExponent));
        slider.onValueChange = [this, &innerSlider]() {
            auto exponent = innerSlider.getValue();
            decayLength.setExponent((float)exponent);
        };
    }


    // ====================================================
    // MultiTimer functions
    // ====================================================

    void timerCallback(int ID) override {
        if (ID == fftTimerID) {
            fftTimerCallback();
            // also do CPU Usage here but move elsewhere later
            updateCPULabel();
        }
    }

    float _lastLevelSent = -10.f; // set to strange value to start off with

    void fftTimerCallback() {
        if (!nextFFTBlockReady) return;
        window.multiplyWithWindowingTable(fftData, fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(fftData);
        auto level = calculateLevel();
        // always show level in history component
        valueHistoryComp.addLevel(level);
        if (sender.isConnected() && level != _lastLevelSent) {
            sender.sendLoudness(level);  // TODO: handle failed sends
            _lastLevelSent = level;
        }
        nextFFTBlockReady = false;
    }

    void updateCPULabel() {
        auto cpu = deviceManager.getCpuUsage() * 100.0f;
        cpuUsageText.setText(String(cpu, 6) + " %", dontSendNotification);
    }

    void startTimerHz(int timerID, float Hz) { startTimer(timerID, (int) (1000.0f / Hz)); }

    // ====================================================

    void logMessage(const String& m) {
        diagnosticsBox.moveCaretToEnd();
        diagnosticsBox.insertTextAtCaret(m + newLine);
    }

    // ===============================
    // OSC functions
    // ===============================

    void setRemoteAddressFromString(const String& text) {
        if (sender.connect(text, 9000)) {
            std::cout
                << "AvvaOSCSender successfully connected to local socket, ready to send to "
                << text << ":9000\n";
            return;
        }
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Connection error",
                                         "Error: could not connect to UDP port 9000.", "OK");
        remoteAddressComp.setText("unable to connect", dontSendNotification);
        std::cout << "Unable to connect to sender at " << text << ":9000\n";
    }

    AvvaOSCSender sender;
    RemoteAddressComponent remoteAddressComp;

    // ===============================

    enum { fftTimerID };
    enum { fftOrder = 8, fftSize = 1 << fftOrder };

    dsp::FFT forwardFFT;
    dsp::WindowingFunction<float> window;

    float fifo[fftSize];
    float fftData[2 * fftSize];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    AudioApp::AudioSourceComponent audioSource;

    Label cpuUsageLabel;
    Label cpuUsageText;
    TextEditor diagnosticsBox;

    ValueShaper valueShaper;
    LabelledSlider shaperInSlider;

    TailOff decayLength;
    LabelledSlider decayLengthSlider;
    static constexpr float initialDecayExponent = 0.5f;

    ValueHistoryComponent valueHistoryComp;
    MovingAverage movingAverage;
    LabelledSlider movingAverageSlider;
    static constexpr int movingAverageInitialWindow = 2;

    LabelledSlider processingBandSlider;
    double processingBandIndexLow = initialProcessingBandLow;
    static constexpr double initialProcessingBandLow = 0.0;
    double processingBandIndexHigh = initialProcessingBandHigh;
    static constexpr double initialProcessingBandHigh = 0.2;

    ToggleButton drawValueHistoryToggle;
    LabelledSlider timerFrequencySlider { "Process rate" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyserComponent)
};
