#include "MainComponent.h"

namespace AudioApp {
    static constexpr float flashProportion = 0.5f;

    MainComponent::MainComponent()
        : analyserComponent([this](float level) { return oscSender.sendLoudness(level); })
    {
        setAudioChannels(2, 2);

        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);

        addAndMakeVisible(uiLogger);
        addAndMakeVisible(audioSource);
        addAndMakeVisible(oscComponent);

        audioSource.onPlaying = [this](){oscSender.sendFilePlaying();};
        audioSource.onPaused = [this](){oscSender.sendFilePaused();};
        audioSource.onStopped = [this](){oscSender.sendFileStopped();};

        tempoAnalyser.onBeat = [this](double period) {
            tempoSynthesizer.beat(period);
            tempoAnalyserFlash.flash(flashProportion * (float) period);
        };

        tempoSynthesizer.onSynthesizedBeat = [this](double period) {
            oscSender.sendClockMillisPerBeat((float)period);
            tempoSynthesizerFlash.flash(flashProportion * (float) period);
        };

        addAndMakeVisible(tempoAnalyserFlash);
        addAndMakeVisible(tempoSynthesizer);
        addAndMakeVisible(tempoSynthesizerFlash);
        addAndMakeVisible(analyserComponent);

        setSize(800, 700);
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

        auto settings = area.removeFromLeft(area.proportionOfWidth(0.5));

        audioSource.setBounds(settings.removeFromTop(410));
        oscComponent.setBounds(settings.removeFromBottom(50));
        uiLogger.setBounds(settings);

        tempoSynthesizer.setBounds(area.removeFromBottom(25));
        auto tempoFlashes = area.removeFromBottom(25);
        tempoAnalyserFlash.setBounds(tempoFlashes.removeFromLeft(25));
        tempoSynthesizerFlash.setBounds(tempoFlashes);

        analyserComponent.setBounds(area);
    }

    void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
        audioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
        tempoAnalyser.updateSamplePerBlockExpected(samplesPerBlockExpected);
    }

    void MainComponent::releaseResources() {
        audioSource.releaseResources();
    }

    void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) {
        if (bufferToFill.buffer->getNumChannels() <= 0)
            return;
        audioSource.getNextAudioBlock(bufferToFill);

        const auto& frameValues = audioSource.getFrameValues();
        tempoAnalyser.processAudioFrame(frameValues);

        for (double sample : frameValues) {
            analyserComponent.pushNextSampleIntoFifo(static_cast<float>(sample));
        }
    }
}