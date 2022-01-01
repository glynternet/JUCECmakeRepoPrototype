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
        auto area = getLocalBounds();
        logger.info("resized: " + area.toString());

        audioSource.setBounds(area.removeFromTop(420));
        oscComponent.setBounds(area.removeFromBottom(50));
        tempoAnalyser.setBounds(area.removeFromBottom(50));
        logger.setBounds(area);
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