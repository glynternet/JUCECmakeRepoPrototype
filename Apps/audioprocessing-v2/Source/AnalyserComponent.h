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
#include "AudioInputSettingsComponent.h"
#include "AvvaOSCSender.h"
#include "FilePlayerTransportComponent.h"
#include "JuceHeader.h"
#include "LabelledSlider.h"
#include "Loudness.h"
#include "MovingAverage.h"
#include "RemoteAddressComponent.h"
#include "TailOff.h"
#include "ValueHistoryComponent.h"
#include "ValueShaper.h"

//==============================================================================
class AnalyserComponent : public AudioAppComponent, private MultiTimer, public ChangeListener {
   public:
    AnalyserComponent()
        : forwardFFT(fftOrder),
          window(fftSize, dsp::WindowingFunction<float>::hann),
          audioDeviceSelectorComp(deviceManager,
                                  0,       // minimum input channels
                                  256,     // maximum input channels
                                  0,       // minimum output channels
                                  256,     // maximum output channels
                                  false,   // ability to select midi inputs
                                  false,   // ability to select midi output device
                                  false,   // treat channels as stereo pairs
                                  false),  // hide advanced options
          filePlayerTransportComp(25),
          processingBandSlider("Frequency Band"),
          shaperInSlider("Range In"),
          valueShaper(0.0f, 1.0f, 0.0f, 1.0f),
          movingAverageSlider("Window Size"),
          movingAverage(movingAverageInitialWindow),
          decayLengthSlider("Decay Length"),
          decayLength(initialDecayExponent),
          drawValueHistoryToggle("Draw Controls"),
          timerFrequencySlider("Process Rate")
    {
        addAndMakeVisible(audioDeviceSelectorComp);

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
            audioDeviceSelectorComp.setVisible(visible);
            diagnosticsBox.setVisible(visible);
            processingBandSlider.setVisible(visible);
            shaperInSlider.setVisible(visible);
            movingAverageSlider.setVisible(visible);
            decayLengthSlider.setVisible(visible);
            filePlayerTransportComp.setVisible(visible);
            audioInputSettingsComp.setVisible(visible);
            remoteAddressComp.setVisible(visible);
            cpuUsageLabel.setVisible(visible);
            cpuUsageText.setVisible(visible);
            timerFrequencySlider.setVisible(visible);
        };

        setSize(700, 500);

        setAudioChannels(2, 2);
        deviceManager.addChangeListener(this);

        setupProcessingBandSlider(processingBandSlider);
        setupShaperInSlider(shaperInSlider);
        setupMovingAverageSlider(movingAverageSlider);
        setupDecayLengthSlider(decayLengthSlider);

        addAndMakeVisible(&filePlayerTransportComp);
        addAndMakeVisible(&audioInputSettingsComp);
        addAndMakeVisible(&remoteAddressComp);
        remoteAddressComp.onTextChange = [this](String text){setRemoteAddressFromString(text);};

        addAndMakeVisible(&timerFrequencySlider);
        constexpr float initialTimerHz = 30.f;
        Slider &slider = timerFrequencySlider.getSlider();
        slider.setRange(5, 60);
        slider.setValue(initialTimerHz,dontSendNotification);
        slider.onValueChange = [this, &slider](){
            startTimerHz(fftTimerID, (float)slider.getValue());
        };
        startTimerHz(fftTimerID, initialTimerHz);

        filePlayerTransportComp.OnPlayPauseClick = [this](FilePlayer::TransportState state){
            if (!sender.isConnected()) {
                std::cout << "Sender not connected. Unable to send play button event message\n";
                return;
            }
            switch (state) {
            case FilePlayer::TransportState::Stopped:
                sender.sendFileStopped();  // TODO: handle failed sends
                break;
            case FilePlayer::TransportState::Starting:
                break;
            case FilePlayer::TransportState::Playing:
                sender.sendFilePlaying();  // TODO: handle failed sends
                break;
            case FilePlayer::TransportState::Pausing:
                break;
            case FilePlayer::TransportState::Paused:
                sender.sendFilePaused();
                break;
            case FilePlayer::TransportState::Stopping:
                break;
            default:
                break;
            }
            return;
        };
    }

    ~AnalyserComponent() {
        deviceManager.removeChangeListener(this);
        shutdownAudio();
    }

    //==============================================================================
    // Component functions

    void resized() override {
        auto rect = getLocalBounds();

        auto valueHistoryRect = rect.removeFromTop(proportionOfHeight(0.5f));
        valueHistoryComp.setBounds(valueHistoryRect);

        // copy valueHistoryRect before doing removes
        auto audioInputSettingsRect = valueHistoryRect;
        audioInputSettingsComp.setBounds(
            audioInputSettingsRect.removeFromBottom(50).removeFromRight(125));
        audioInputSettingsComp.addChangeListener(this);

        audioSettingsRect = rect;
        resizeAudioSettings(audioSettingsRect);

        resizeSliderGroup();

        // copy valueHistoryRect before doing removes
        auto transportControlRect = valueHistoryRect;
        filePlayerTransportComp.setBounds(
            transportControlRect.removeFromBottom(70).removeFromLeft(125));

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
        audioDeviceSelectorComp.setBounds(adscBounds);

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
        filePlayerTransportComp.getFilePlayer().prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override { filePlayerTransportComp.getFilePlayer().releaseResources(); }

    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override {
        if (bufferToFill.buffer->getNumChannels() <= 0) return;

        switch (audioInputSettingsComp.getSelectedInput()) {
        case AudioInputSettingsComponent::AudioInput::noneSelected:
                // output nothing
                bufferToFill.clearActiveBufferRegion();
                return;
            case AudioInputSettingsComponent::AudioInput::fromFile: {
                filePlayerTransportComp.getFilePlayer().getNextAudioBlock(bufferToFill);
            } break;
            case AudioInputSettingsComponent::AudioInput::fromDevice:
                // buffer has already been filled with input data
                break;
            default:
                // TODO: throw something?
                return;
        }

        // Currently only processing a single channel
        auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

        for (auto i = 0; i < bufferToFill.numSamples; ++i) {
            pushNextSampleIntoFifo(channelData[i]);
        }

        if (!audioInputSettingsComp.isMonitoringInput()) {
            bufferToFill.clearActiveBufferRegion();
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
            if (!nextFFTBlockReady) {
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
    // ChangeListener functions
    // ====================================================

    void changeListenerCallback(ChangeBroadcaster* source) override {
        if (source == &deviceManager) {
            deviceManagerChangeCallback();
            return;
        }

        if (source == &audioInputSettingsComp) {
            updateFilePlayerState();
        }
    }

    void updateFilePlayerState() {
        switch (audioInputSettingsComp.getSelectedInput()) {
            case AudioInputSettingsComponent::AudioInput::noneSelected:
                filePlayerTransportComp.setVisible(false);
                return;
            case AudioInputSettingsComponent::AudioInput::fromFile: {
                filePlayerTransportComp.setVisible(true);
            } break;
            case AudioInputSettingsComponent::AudioInput::fromDevice:
                filePlayerTransportComp.setVisible(false);
                break;
            default:
                // TODO: throw something?
                return;
        }
    }

    void deviceManagerChangeCallback() { dumpDeviceInfo(); }

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

    float HzToPeriodMilliseconds(float Hz) { return 1000.0f / Hz; }

    void startTimerHz(int timerID, float Hz) { startTimer(timerID, (int)HzToPeriodMilliseconds(Hz)); }

    // ====================================================

    void dumpDeviceInfo() {
        diagnosticsBox.clear();
        logMessage("Current audio device type: " +
                   (deviceManager.getCurrentDeviceTypeObject() != nullptr
                        ? deviceManager.getCurrentDeviceTypeObject()->getTypeName()
                        : "<none>"));

        if (auto* device = deviceManager.getCurrentAudioDevice()) {
            logMessage("Current audio device: " + device->getName().quoted());
            logMessage("Sample rate: " + String(device->getCurrentSampleRate()) + " Hz");
            logMessage("Block size: " + String(device->getCurrentBufferSizeSamples()) + " samples");
            logMessage("Bit depth: " + String(device->getCurrentBitDepth()));
            logMessage("Active input channels: " +
                       getListOfActiveBits(device->getActiveInputChannels()));
        } else {
            logMessage("No audio device open");
        }
    }

    void logMessage(const String& m) {
        diagnosticsBox.moveCaretToEnd();
        diagnosticsBox.insertTextAtCaret(m + newLine);
    }

    static String getListOfActiveBits(const BigInteger& b) {
        StringArray bits;

        for (auto i = 0; i <= b.getHighestBit(); ++i)
            if (b[i]) bits.add(String(i));

        return bits.joinIntoString(", ");
    }

    // ===============================
    // OSC functions
    // ===============================

    void setRemoteAddressFromString(String text) {
        if (sender.connect(text, 9000)) {
            std::cout
                << "AvvaOSCSender successfully connected to local socket, ready to send to "
                << text << ":9000\n";
            return;
        }
        showConnectionErrorMessage("Error: could not connect to UDP port 9000.");
        remoteAddressComp.setText("unable to connect", dontSendNotification);
        std::cout << "Unable to connect to sender at " << text << ":9000\n";
    }

    void showConnectionErrorMessage(const String& messageText) {
         AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Connection error", messageText,
                                         "OK");
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

    AudioDeviceSelectorComponent audioDeviceSelectorComp;
    Rectangle<int> audioSettingsRect;
    AudioInputSettingsComponent audioInputSettingsComp;
    FilePlayerTransportComponent filePlayerTransportComp;

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
    LabelledSlider timerFrequencySlider;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyserComponent)
};
