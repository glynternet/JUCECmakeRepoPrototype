#include "MainComponent.h"

namespace AudioApp
{
MainComponent::MainComponent()
{
    setAudioChannels(2,0);
    addAndMakeVisible(selector);
    setSize(600, 400);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.drawFittedText(std::to_string(tempo), getLocalBounds().removeFromBottom(50), juce::Justification::horizontallyCentred, 1);
}

void MainComponent::resized()
{
    selector.setBounds(getLocalBounds().withTrimmedBottom(50));
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::Logger::writeToLog("Preparing to play: "+std::to_string(samplesPerBlockExpected)+" "+std::to_string(sampleRate));
    juce::ignoreUnused(samplesPerBlockExpected, sampleRate);
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
        juce::Logger::writeToLog("Beats: "+ std::to_string(++count));
        tempo = b.getCurrentTempoEstimate();
        juce::Logger::writeToLog("Tempo estimate: "+ std::to_string(b.getCurrentTempoEstimate()));
        juce::Logger::writeToLog("Cumulative score: "+ std::to_string(b.getLatestCumulativeScoreValue()));
    }

    bufferToFill.clearActiveBufferRegion();
}
}