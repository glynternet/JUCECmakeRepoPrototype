#include "AudioSourceComponent.h"

namespace AudioApp {

#define BUTTONS_GAP 10
#define BUTTONS_HEIGHT 30

    AudioSourceComponent::AudioSourceComponent(juce::AudioDeviceManager &deviceManager, Logger &logger)
            : deviceManager(deviceManager), logger(logger) {

        openButton.onClick = [this] { openButtonClicked(); };
        addAndMakeVisible(&openButton);

        playPauseButton.onClick = [this] { transportStateChanged(Starting); };
        playPauseButton.setEnabled(true);
        addAndMakeVisible(&playPauseButton);

        stopButton.onClick = [this] { transportStateChanged(Stopping); };
        stopButton.setEnabled(false);
        addAndMakeVisible(&stopButton);

        formatManager.registerBasicFormats();
        transport.addChangeListener(this);

        addAndMakeVisible(selector);

        sourceToggle.onStateChange = [this] {
            bool newFilePlayerEnabled = sourceToggle.getToggleState();
            if (newFilePlayerEnabled != filePlayerEnabled) {
                filePlayerEnabled = newFilePlayerEnabled;
                this->logger.debug("Updated filePlayerEnabled to: " + std::to_string(filePlayerEnabled));
                if (filePlayerEnabled) {
                    playPauseButton.setVisible(true);
                    stopButton.setVisible(true);
                    openButton.setVisible(true);
                } else {
                    if (state != Stopped) {
                        transportStateChanged(Stopping);
                    }
                    playPauseButton.setVisible(false);
                    stopButton.setVisible(false);
                    openButton.setVisible(false);
                }
                resized();
            }
        };
        addAndMakeVisible(&sourceToggle);

        addAndMakeVisible(&monitorOutputToggle);
    }

    void AudioSourceComponent::paint(juce::Graphics &) {}

    void AudioSourceComponent::resized() {
        auto bounds = getLocalBounds();

        if (filePlayerEnabled) {
            bounds.removeFromBottom(BUTTONS_GAP);
            auto transportButtonsBounds = bounds.removeFromBottom(BUTTONS_HEIGHT)
                    .withTrimmedLeft(BUTTONS_GAP)
                    .withTrimmedRight(BUTTONS_GAP);
            auto buttonWidth = (transportButtonsBounds.getWidth() - 2 * BUTTONS_GAP) / 3;
            openButton.setBounds(transportButtonsBounds.removeFromLeft(buttonWidth));
            transportButtonsBounds.removeFromLeft(BUTTONS_GAP);
            playPauseButton.setBounds(transportButtonsBounds.removeFromLeft(buttonWidth));
            transportButtonsBounds.removeFromLeft(BUTTONS_GAP);
            stopButton.setBounds(transportButtonsBounds.removeFromLeft(buttonWidth));
        }

        bounds.removeFromBottom(BUTTONS_GAP);
        sourceToggle.setBounds(bounds.removeFromBottom(BUTTONS_HEIGHT)
                                       .withTrimmedRight(BUTTONS_GAP)
                                       .withTrimmedLeft(BUTTONS_GAP));
        monitorOutputToggle.setBounds(bounds.removeFromBottom(BUTTONS_HEIGHT)
                                              .withTrimmedRight(BUTTONS_GAP)
                                              .withTrimmedLeft(BUTTONS_GAP));

        selector.setBounds(bounds);
    }

    void AudioSourceComponent::changeListenerCallback(juce::ChangeBroadcaster *source) {
        logger.debug("Change listener callback triggered");
        if (source == &transport) {
            if (transport.isPlaying()) {
                transportStateChanged(Playing);
            } else if (state == Pausing) {
                transportStateChanged(Paused);
            } else {
                // file transport has reached end or stopped for some other reason
                transportStateChanged(Stopped);
            }
        }
    }

    void AudioSourceComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
        logger.info("Preparing to play");
        transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void AudioSourceComponent::releaseResources() {
        // I THINK THIS IS NOT NEEDED BECAUSE RESOURCES ARE RELEASED IN THE MAINCOMPONENT
    }

    void AudioSourceComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) {
        if (filePlayerEnabled) {
            transport.getNextAudioBlock(bufferToFill);

            if (bufferToFill.buffer->getNumChannels() == 0) {
                logger.error("No channels in buffer to fill");
                bufferToFill.clearActiveBufferRegion();
                return;
            }
        } else {
            // example from https://docs.juce.com/master/tutorial_processing_audio_input.html
            auto *device = deviceManager.getCurrentAudioDevice();
            const auto activeInputChannels = device->getActiveInputChannels();

            // BigInteger::getHighestBit returns -1 when value is 0,
            // where no input channels would be available.
            if (activeInputChannels.getHighestBit() == -1) {
                std::vector<double> frameValues(bufferToFill.numSamples);
                std::fill(frameValues.begin(), frameValues.end(), 0);
                frameValues2 = frameValues;
                return;
            }
            if (!activeInputChannels[0]) {
                std::vector<double> frameValues(bufferToFill.numSamples);
                std::fill(frameValues.begin(), frameValues.end(), 0);
                frameValues2 = frameValues;
                return;
            }
        }

        const auto *inputData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

        // as prescribed in BTrack README: https://github.com/adamstark/BTrack
        // TODO(glynternet): is there a float version of BTrack so that we can avoid this conversion of float to double and avoid creating a vector?
        // TODO(glynternet): if the above todo is not possible, can we reuse this vector to avoid having to create a new one every audio block?
        std::vector<double> frameValues(bufferToFill.numSamples);

        // TODO(glynternet): is it quicker to use std::copy here?
        for (auto i = 0; i < bufferToFill.numSamples; ++i) {
            frameValues[i] = inputData[i];
        }

        // TODO(glynternet): profile using getToggleState vs saving to a bool when state changes and then reading the bool
        if (!monitorOutputToggle.getToggleState()) {
            bufferToFill.clearActiveBufferRegion();
        }

        frameValues2 = frameValues;
    }

    double *AudioSourceComponent::getFrameValues() {
        return frameValues2.data();
    }

    void AudioSourceComponent::openButtonClicked() {
        logger.debug("Open button clicked");
        fileChooser_ = std::make_unique<juce::FileChooser>(("Choose a supported file to play..."),
                                                           juce::File::getSpecialLocation(
                                                                   juce::File::userMusicDirectory),
                // TODO: MP3 doesn't seem to work on z30-a linux.
                //   Does it work on any other machine OS combo?
                                                           "*.wav;*.aiff;*.aif;*.flac");

        fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this](const juce::FileChooser &fileChooser) {
                                      this->chooserClosed(fileChooser);
                                  });
    }

    void AudioSourceComponent::chooserClosed(const juce::FileChooser &chooser) {
        juce::File file(chooser.getResult());
        logger.debug("File chooser closed");

        juce::AudioFormatReader *reader = formatManager.createReaderFor(file);
        if (reader != nullptr) {
            //get the file ready to play
            std::unique_ptr<juce::AudioFormatReaderSource> sourceReader(
                    new juce::AudioFormatReaderSource(reader, true));

            transport.setSource(sourceReader.get());
            transportStateChanged(Stopped);

            // I believe this needs to be here so that the tempSource is not deleted until playSource is deleted,
            // which will happen when the AudioSourceComponent is deleted.
            playSource = std::move(sourceReader);
        }
        logger.debug("Reader prepared");
    }

    void AudioSourceComponent::transportStateChanged(TransportState newState) {
        if (newState != state) {
            state = newState;

            switch (state) {
                case Stopped:
                    logger.info("Stopped");
                    playPauseButton.setButtonText("Play");
                    playPauseButton.setEnabled(true);
                    playPauseButton.onClick = [this](){
                        transportStateChanged(Starting);
                    };
                    stopButton.setEnabled(false);
                    transport.setPosition(0.0);
                    break;

                case Paused:
                    logger.info("Paused");
                    playPauseButton.setButtonText("Play");
                    playPauseButton.setEnabled(true);
                    playPauseButton.onClick = [this](){
                        transportStateChanged(Starting);
                    };
                    // TODO(glynternet): handle clicking stop when paused:
                    //  will probably need to handle this without a transport changed call back
                    stopButton.setEnabled(false);
                    break;

                case Starting:
                    logger.info("Starting");
                    stopButton.setEnabled(true);
                    playPauseButton.setEnabled(false);
                    transport.start();
                    break;

                case Playing:
                    logger.info("Playing");
                    playPauseButton.setButtonText("Pause");
                    playPauseButton.onClick = [this](){
                        transportStateChanged(Pausing);
                    };
                    playPauseButton.setEnabled(true);
                    stopButton.setEnabled(true);
                    break;

                case Stopping:
                    logger.info("Stopping");
                    playPauseButton.setEnabled(false);
                    stopButton.setEnabled(false);
                    transport.stop();
                    break;

                case Pausing:
                    logger.info("Pausing");
                    playPauseButton.setEnabled(false);
                    stopButton.setEnabled(false);
                    transport.stop();
                    break;
            }
        }
    }
}