#include "MainComponent.h"

namespace AudioApp
{
    MainComponent::MainComponent() {
        setAudioChannels(2,2);

        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);

        addAndMakeVisible(logger);
        addAndMakeVisible(audioSource);
        addAndMakeVisible(oscComponent);

        tempoAnalyser.onBeat = [this](double period) {
            oscComponent.sendBeatMessage();
            tempoSynthesizer.beat(period);
        };
        addAndMakeVisible(tempoAnalyser);
        addAndMakeVisible(tempoSynthesizer);

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
        logger.debug("resized: " + area.toString());

        audioSource.setBounds(area.removeFromTop(390));
        oscComponent.setBounds(area.removeFromBottom(50));
        tempoSynthesizer.setBounds(area.removeFromBottom(50));
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