#include "MainComponent.h"

namespace AudioApp {
#define FLASH_PROPORTION 0.5f

    MainComponent::MainComponent() {
        setAudioChannels(2, 2);

        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);

        addAndMakeVisible(logger);
        addAndMakeVisible(audioSource);
        addAndMakeVisible(oscComponent);

        audioSource.onPlaying = [this](){oscSender.sendFilePlaying();};
        audioSource.onPaused = [this](){oscSender.sendFilePaused();};
        audioSource.onStopped = [this](){oscSender.sendFileStopped();};

        tempoAnalyser.onBeat = [this](double period) {
            tempoSynthesizer.beat(period);
            tempoAnalyserFlash.flash(FLASH_PROPORTION * (float) period);
        };

        tempoSynthesizer.onSynthesizedBeat = [this](double period) {
            oscSender.sendClockMillisPerBeat((float)period);
            tempoSynthesizerFlash.flash(FLASH_PROPORTION * (float) period);
        };
        addAndMakeVisible(tempoAnalyserFlash);
        addAndMakeVisible(tempoSynthesizer);
        addAndMakeVisible(tempoSynthesizerFlash);

        setSize(400, 700);
    }

    MainComponent::~MainComponent() {
        shutdownAudio();
    }

    void MainComponent::paint(juce::Graphics &g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void MainComponent::resized() {
        auto area = getLocalBounds();
        logger.debug("resized: " + area.toString());

        audioSource.setBounds(area.removeFromTop(410));
        oscComponent.setBounds(area.removeFromBottom(50));
        tempoSynthesizer.setBounds(area.removeFromBottom(25));
        auto tempoFlashes = area.removeFromBottom(25);
        tempoAnalyserFlash.setBounds(tempoFlashes.removeFromLeft(25));
        tempoSynthesizerFlash.setBounds(tempoFlashes);
        logger.setBounds(area);
    }

    void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
        audioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
        tempoAnalyser.updateSamplePerBlockExpected(samplesPerBlockExpected);
    }

    void MainComponent::releaseResources() {}

    void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) {
        audioSource.getNextAudioBlock(bufferToFill);
        tempoAnalyser.processAudioFrame(audioSource.getFrameValues());
    }
}