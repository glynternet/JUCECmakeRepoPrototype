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
    tempoLabel.setText(std::to_string(tempo), juce::dontSendNotification);
    if (beatDetectionPaintRequired) {
        beatDetectionPaintRequired = false;
        if (tempoLabel.findColour(juce::Label::textColourId) == juce::Colours::lightgreen) {
            tempoLabel.setColour (juce::Label::textColourId, juce::Colours::darkblue);
        } else {
            tempoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
        }
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
        juce::Logger::writeToLog("Beat due in current frame");
        beatDetectionPaintRequired = true;
        tempo = b.getCurrentTempoEstimate();
        juce::Logger::writeToLog("Tempo estimate: "+ std::to_string(b.getCurrentTempoEstimate()));
        juce::Logger::writeToLog("Cumulative score: "+ std::to_string(b.getLatestCumulativeScoreValue()));
    }

    bufferToFill.clearActiveBufferRegion();
}
}