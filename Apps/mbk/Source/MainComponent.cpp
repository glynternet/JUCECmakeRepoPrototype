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
        logger.setBounds(settings);

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

        tempoAnalyser.processAudioFrame(audioSource.getFrameValues());

        // TODO(glynternet): This isn't great, we're copying all of the frames to another vector just to extract them.
        //   We either want to hook directly into getNextAudioBlock or take the frame values and push all of
        //   them at the same time into the Fifo buffer? Not sure, a thing for another day.
        double* frameValues = audioSource.getFrameValues();
        for (auto i = 0; i < bufferToFill.numSamples; ++i)
        {
            analyserComponent.pushNextSampleIntoFifo(frameValues[i]);
        }
    }
}