#include "AudioSourceComponent.h"

namespace AudioApp {

#define BUTTONS_GAP 10
#define BUTTONS_HEIGHT 30

    AudioSourceComponent::AudioSourceComponent(juce::AudioDeviceManager &deviceManager, logger::Logger &logger)
            : deviceManager(deviceManager), logger(logger) {

        openButton.onClick = [this] { openFileChooser(); };
        addAndMakeVisible(&openButton);
        playPauseButton.setEnabled(false);
        addAndMakeVisible(&playPauseButton);
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

        cpuUsageText.setJustificationType(juce::Justification::left);
        addAndMakeVisible(&cpuUsageLabel);
        addAndMakeVisible(&cpuUsageText);

        startTimerHz(30.f);
    }

    void AudioSourceComponent::paint(juce::Graphics &) {}

    void AudioSourceComponent::resized() {
        auto bounds = getLocalBounds();

        auto cpuSpace = bounds.removeFromBottom(20);
        cpuUsageText.setBounds(cpuSpace.removeFromRight(100));
        cpuUsageLabel.setBounds(cpuSpace);

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

    void AudioSourceComponent::timerCallback() {
        const auto cpuPercent = deviceManager.getCpuUsage() * 100.0f;
        cpuUsageText.setText(juce::String(cpuPercent, 6) + " %", juce::dontSendNotification);
    }

    void AudioSourceComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
        logger.info("Preparing to play: samplesPerBlockExpected="+std::to_string(samplesPerBlockExpected)+" sampleRate="+std::to_string(sampleRate));
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
                std::vector<double> silence(bufferToFill.numSamples);
                std::fill(silence.begin(), silence.end(), 0);
                frame = silence;
                bufferToFill.clearActiveBufferRegion();
                return;
            }
        } else {
            // example from https://docs.juce.com/master/tutorial_processing_audio_input.html
            const auto activeInputChannels =
                deviceManager.getCurrentAudioDevice()->getActiveInputChannels();

            // BigInteger::getHighestBit returns -1 when value is 0,
            // where no input channels would be available.
            if (activeInputChannels.getHighestBit() == -1 || !activeInputChannels[0]) {
                logger.error("No input channels");
                std::vector<double> silence(bufferToFill.numSamples);
                std::fill(silence.begin(), silence.end(), 0);
                frame = silence;
                bufferToFill.clearActiveBufferRegion();
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

        frame = frameValues;
    }

    double *AudioSourceComponent::getFrameValues() {
        return frame.data();
    }

    void AudioSourceComponent::openFileChooser() {
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
        juce::AudioFormatReader *reader = formatManager.createReaderFor(file);
        if (reader == nullptr) {
            logger.info("No file chosen");
            return;
        }

        std::unique_ptr<juce::AudioFormatReaderSource> sourceReader(
                new juce::AudioFormatReaderSource(reader, true));

        transport.setSource(sourceReader.get());
        transportStateChanged(Stopped);

        double fileSampleRate = reader->sampleRate;
        if (deviceManager.getCurrentAudioDevice()->getCurrentSampleRate() == fileSampleRate) {
            logger.info("File loaded: name=" +
                        file.getFullPathName().toStdString()
                        + " format="+reader->getFormatName().toStdString()
                        + " sampleRate="+std::to_string(fileSampleRate));
            return;
        }

        const juce::Array<double>& supportedSampleRates =
            deviceManager.getCurrentAudioDevice()->getAvailableSampleRates();
        if (!supportedSampleRates.contains(fileSampleRate)) {
            std::string msg =
                "Current audio device does not support file sample rate, try changing audio device then reloading file: fileSampleRate="
                    +std::to_string(fileSampleRate)+" supportedSampleRates:[";
            switch (supportedSampleRates.size()) {
                case 0:
                    break;
                case 1:
                    msg += std::to_string(supportedSampleRates.getFirst());
                    break;
                default:
                    msg += std::reduce(supportedSampleRates.begin(), supportedSampleRates.end(), (std::string)"", [](const std::string& accum, double next){
                      return accum + (accum.empty() ? "" : ",") + std::to_string(next);
                    }) + "]";
                    break;
            }
            logger.error(msg);
            return;
        }

        // I believe this needs to be here so that the tempSource is not deleted until playSource is deleted,
        // which will happen when the AudioSourceComponent is deleted.
        playSource = std::move(sourceReader);

        juce::AudioDeviceManager::AudioDeviceSetup deviceSetup = deviceManager.getAudioDeviceSetup();
        deviceSetup.sampleRate = fileSampleRate;
        const std::string& deviceSetupUpdateErrorMessage =
            deviceManager.setAudioDeviceSetup(deviceSetup, true).toStdString();
        if (!deviceSetupUpdateErrorMessage.empty()) {
            logger.error("Error updating device sample rate to match file ("+std::to_string(fileSampleRate)+"): "+deviceSetupUpdateErrorMessage);
            return;
        }

        logger.info("File loaded and sample rate updated: name=" +
                    file.getFullPathName().toStdString()
                    + " format="+reader->getFormatName().toStdString()
                    + " sampleRate="+std::to_string(fileSampleRate));
    }

    void AudioSourceComponent::transportStateChanged(TransportState newState) {
        if (newState != state) {
            state = newState;

            switch (state) {
                case Stopped:
                    logger.info("Stopped");
                    if (onStopped != nullptr) onStopped();
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
                    if (onPaused != nullptr) onPaused();
                    playPauseButton.setButtonText("Play");
                    playPauseButton.setEnabled(true);
                    playPauseButton.onClick = [this](){
                        transportStateChanged(Starting);
                    };
                    stopButton.setEnabled(true);
                    stopButton.onClick = [this]() {
                        transportStateChanged(Stopped);
                    };
                    break;

                case Starting:
                    logger.info("Starting");
                    playPauseButton.setEnabled(false);
                    stopButton.setEnabled(true);
                    transport.start();
                    break;

                case Playing:
                    logger.info("Playing");
                    if (onPlaying != nullptr) onPlaying();
                    playPauseButton.setButtonText("Pause");
                    playPauseButton.setEnabled(true);
                    playPauseButton.onClick = [this](){
                        transportStateChanged(Pausing);
                    };
                    stopButton.setEnabled(true);
                    stopButton.onClick = [this]() {
                        transportStateChanged(Stopping);
                    };
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