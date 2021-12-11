#include "MainComponent.h"

namespace AudioApp
{
    MainComponent::MainComponent()
    {
        setAudioChannels(2,2);

        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);

        addAndMakeVisible(logger);
        addAndMakeVisible(audioSource);
        addAndMakeVisible(oscComponent);

        tempoAnalyser.onBeat = [this] { oscComponent.sendBeatMessage(); };
        addAndMakeVisible(tempoAnalyser);

        setSize (400, 700);
    }

    MainComponent::~MainComponent()
    {
        shutdownAudio();
    }

    void MainComponent::paint(Graphics& g)
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void MainComponent::resized()
    {
        logger.info("resized: " + getLocalBounds().toString());
        audioSource.setBounds(getLocalBounds().removeFromTop(420));
        logger.setBounds(getLocalBounds()
            .withTrimmedBottom(50)
            .withTrimmedTop(420));
        tempoAnalyser.setBounds(getLocalBounds().removeFromBottom(50));
        oscComponent.setBounds(getLocalBounds().removeFromBottom(50).translated(0, -50));
    }

    void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
    {
        audioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
        tempoAnalyser.updateSamplePerBlockExpected(samplesPerBlockExpected);
    }

    void MainComponent::releaseResources() {}

    void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        audioSource.getNextAudioBlock(bufferToFill);
        tempoAnalyser.processAudioFrame(audioSource.getFrameValues());
    }
}