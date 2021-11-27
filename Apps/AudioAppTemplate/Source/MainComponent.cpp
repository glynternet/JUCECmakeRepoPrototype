#include "MainComponent.h"

namespace AudioApp
{
MainComponent::MainComponent()
{
    setAudioChannels(2,0);

    addAndMakeVisible(selector);

    tempoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
    tempoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(tempoLabel);

    setSize(600, 400);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    tempoLabel.setText(tempo, juce::dontSendNotification);
    if (paintBeatDetectionBright) {
        tempoLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    } else {
        tempoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    }
}

void MainComponent::resized()
{
    selector.setBounds(getLocalBounds().withTrimmedBottom(50));
    tempoLabel.setBounds(getLocalBounds().removeFromBottom(50));
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    btrackFrameSize = samplesPerBlockExpected;
    btrackHopSize = samplesPerBlockExpected / 2;
    b.updateHopAndFrameSize(btrackHopSize, btrackFrameSize);
    juce::Logger::writeToLog("Updated BTrack to with new hopSize:"+std::to_string(btrackHopSize)+" and frameSize:"+std::to_string(btrackFrameSize));
}

void MainComponent::releaseResources()
{
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (bufferToFill.numSamples != btrackFrameSize) {
        juce::Logger::writeToLog("Num samples not equal to frame size: frameSize:" + std::to_string(btrackFrameSize) + " numSamples:" + std::to_string(bufferToFill.numSamples));
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (bufferToFill.buffer->getNumChannels() == 0) {
        juce::Logger::writeToLog("No channels in buffer to fill");
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    const auto* channelData = bufferToFill.buffer->getReadPointer (0, bufferToFill.startSample);

    // as prescribed in BTrack README: https://github.com/adamstark/BTrack
    double frameValues[btrackFrameSize];

    for (auto i = 0; i < bufferToFill.numSamples; ++i) {
        frameValues[i] = channelData[i];
    }

    b.processAudioFrame(frameValues);
    if (b.beatDueInCurrentFrame()) {
        paintBeatDetectionBright = true;
        juce::Timer::callAfterDelay(250, [this]{this->paintBeatDetectionBright = false;});
        tempo = juce::String(b.getCurrentTempoEstimate());
    }

    bufferToFill.clearActiveBufferRegion();
}
}