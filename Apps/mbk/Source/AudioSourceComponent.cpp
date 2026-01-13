#include "AudioSourceComponent.h"
#include "prettyprint.hpp"
#include <algorithm>

namespace AudioApp {
static constexpr int buttonsGap = 10;
static constexpr int buttonsHeight = 30;

AudioSourceComponent::AudioSourceComponent(juce::AudioDeviceManager& deviceManager,
                                           logger::Logger& logger)
    : deviceManager(deviceManager)
    , logger(logger) {
    openButton.onClick = [this] { openFileChooser(); };
    addAndMakeVisible(&openButton);
    playPauseButton.setEnabled(false);
    addAndMakeVisible(&playPauseButton);
    stopButton.setEnabled(false);
    addAndMakeVisible(&stopButton);

    formatManager.registerBasicFormats();
    transport.addChangeListener(this);
    deviceManager.addChangeListener(this);

    addAndMakeVisible(selector);

    sourceToggle.onStateChange = [this] {
        bool newFilePlayerEnabled = sourceToggle.getToggleState();
        if (newFilePlayerEnabled != filePlayerEnabled) {
            filePlayerEnabled = newFilePlayerEnabled;
            this->logger.debug("Updated filePlayerEnabled to: "
                               + std::string(filePlayerEnabled ? "true" : "false"));
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

    startTimerHz(30);

    auto currentSetup = deviceManager.getAudioDeviceSetup();
    lastInputDeviceName = currentSetup.inputDeviceName;
    setMinimumBufferSize();
}

void AudioSourceComponent::paint(juce::Graphics& /*graphics*/) {
}

void AudioSourceComponent::resized() {
    auto bounds = getLocalBounds();

    auto cpuSpace = bounds.removeFromBottom(20);
    cpuUsageText.setBounds(cpuSpace.removeFromRight(100));
    cpuUsageLabel.setBounds(cpuSpace);

    if (filePlayerEnabled) {
        bounds.removeFromBottom(buttonsGap);
        auto transportButtonsBounds = bounds.removeFromBottom(buttonsHeight)
                                          .withTrimmedLeft(buttonsGap)
                                          .withTrimmedRight(buttonsGap);
        auto buttonWidth = (transportButtonsBounds.getWidth() - 2 * buttonsGap) / 3;
        openButton.setBounds(transportButtonsBounds.removeFromLeft(buttonWidth));
        transportButtonsBounds.removeFromLeft(buttonsGap);
        playPauseButton.setBounds(transportButtonsBounds.removeFromLeft(buttonWidth));
        transportButtonsBounds.removeFromLeft(buttonsGap);
        stopButton.setBounds(transportButtonsBounds.removeFromLeft(buttonWidth));
    }

    bounds.removeFromBottom(buttonsGap);
    sourceToggle.setBounds(bounds.removeFromBottom(buttonsHeight)
                               .withTrimmedRight(buttonsGap)
                               .withTrimmedLeft(buttonsGap));
    monitorOutputToggle.setBounds(bounds.removeFromBottom(buttonsHeight)
                                      .withTrimmedRight(buttonsGap)
                                      .withTrimmedLeft(buttonsGap));

    selector.setBounds(bounds);
}

void AudioSourceComponent::changeListenerCallback(juce::ChangeBroadcaster* source) {
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
    } else if (source == &deviceManager) {
        auto currentSetup = deviceManager.getAudioDeviceSetup();
        juce::String currentInputDevice = currentSetup.inputDeviceName;

        if (currentInputDevice != lastInputDeviceName) {
            logger.info("Input device changed from '" + lastInputDeviceName.toStdString()
                        + "' to '" + currentInputDevice.toStdString() + "'");
            lastInputDeviceName = currentInputDevice;
            noInputChannelsLimiter.reset();
            // Defer to run after AudioDeviceSelectorComponent finishes restoring settings
            juce::MessageManager::callAsync([this]() { setMinimumBufferSize(); });
        }
    }
}

void AudioSourceComponent::timerCallback() {
    const auto cpuPercent = deviceManager.getCpuUsage() * 100.0F;
    cpuUsageText.setText(juce::String(cpuPercent, 6) + " %", juce::dontSendNotification);
}

void AudioSourceComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    logger.info("Preparing to play: samplesPerBlockExpected="
                + std::to_string(samplesPerBlockExpected)
                + " sampleRate=" + std::to_string(sampleRate));
    transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioSourceComponent::releaseResources() {
    // I THINK THIS IS NOT NEEDED BECAUSE RESOURCES ARE RELEASED IN THE MAINCOMPONENT
}

void AudioSourceComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
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
            if (noInputChannelsLimiter.allowNow()) {
                logger.error("No input channels");
            }
            std::vector<double> silence(bufferToFill.numSamples);
            std::fill(silence.begin(), silence.end(), 0);
            frame = silence;
            bufferToFill.clearActiveBufferRegion();
            return;
        }
    }

    const auto* inputData =
        bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

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

    {
        const juce::SpinLock::ScopedLockType lock(frameLock);
        frame = frameValues;
    }
}

const std::vector<double>& AudioSourceComponent::getFrameValues() {
    // Note: caller must ensure they finish using the returned reference before
    // the next audio block is processed. Consider copying data instead.
    const juce::SpinLock::ScopedLockType lock(frameLock);
    return frame;
}

void AudioSourceComponent::openFileChooser() {
    logger.debug("Open button clicked");
    fileChooser_ = std::make_unique<juce::FileChooser>(
        ("Choose a supported file to play..."),
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        // TODO: MP3 doesn't seem to work on z30-a linux.
        //   Does it work on any other machine OS combo?
        "*.wav;*.aiff;*.aif;*.flac");

    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [this](const juce::FileChooser& fileChooser) {
                                  this->chooserClosed(fileChooser);
                              });
}

void AudioSourceComponent::chooserClosed(const juce::FileChooser& chooser) {
    juce::File file(chooser.getResult());
    juce::AudioFormatReader* reader = formatManager.createReaderFor(file);
    if (reader == nullptr) {
        logger.info("No file chosen");
        return;
    }

    // Store source reader to maintain lifetime - transport.setSource takes a raw pointer
    // and doesn't take ownership, so we must keep the source alive
    playSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

    transport.setSource(playSource.get());
    transportStateChanged(Stopped);

    double fileSampleRate = reader->sampleRate;
    if (deviceManager.getCurrentAudioDevice()->getCurrentSampleRate() == fileSampleRate) {
        logger.info("File loaded: name=" + file.getFullPathName().toStdString()
                    + " format=" + reader->getFormatName().toStdString()
                    + " sampleRate=" + std::to_string(fileSampleRate));
        return;
    }

    const juce::Array<double>& supportedSampleRates =
        deviceManager.getCurrentAudioDevice()->getAvailableSampleRates();
    if (supportedSampleRates.size() == 0) {
        logger.error(
            "Current audio device does not have any supported sample rates: fileSampleRate="
            + std::to_string(fileSampleRate) + " supportedSampleRates:");
        return;
    }
    if (!supportedSampleRates.contains(fileSampleRate)) {
        std::string msg =
            "Current audio device does not support file sample rate, try changing audio device then reloading file: fileSampleRate="
            + std::to_string(fileSampleRate) + " supportedSampleRates:";
        // TODO(glynternet): how do we make std::to_string work with this instead?
        std::ostringstream oss;
        oss << std::vector<double>(supportedSampleRates.begin(),
                                   supportedSampleRates.end());
        msg += oss.str();
        logger.error(msg);
        return;
    }

    juce::AudioDeviceManager::AudioDeviceSetup deviceSetup =
        deviceManager.getAudioDeviceSetup();
    deviceSetup.sampleRate = fileSampleRate;
    const std::string& deviceSetupUpdateErrorMessage =
        deviceManager.setAudioDeviceSetup(deviceSetup, true).toStdString();
    if (!deviceSetupUpdateErrorMessage.empty()) {
        logger.error("Error updating device sample rate to match file ("
                     + std::to_string(fileSampleRate)
                     + "): " + deviceSetupUpdateErrorMessage);
        return;
    }

    logger.info("File loaded and sample rate updated: name="
                + file.getFullPathName().toStdString()
                + " format=" + reader->getFormatName().toStdString()
                + " sampleRate=" + std::to_string(fileSampleRate));
}

void AudioSourceComponent::transportStateChanged(TransportState newState) {
    if (newState != state) {
        state = newState;

        switch (state) {
            case Stopped:
                logger.info("Stopped");
                if (onStopped != nullptr) {
                    onStopped();
                }
                playPauseButton.setButtonText("Play");
                playPauseButton.setEnabled(true);
                playPauseButton.onClick = [this]() { transportStateChanged(Starting); };
                stopButton.setEnabled(false);
                transport.setPosition(0.0);
                break;

            case Paused:
                logger.info("Paused");
                if (onPaused != nullptr) {
                    onPaused();
                }
                playPauseButton.setButtonText("Play");
                playPauseButton.setEnabled(true);
                playPauseButton.onClick = [this]() { transportStateChanged(Starting); };
                stopButton.setEnabled(true);
                stopButton.onClick = [this]() { transportStateChanged(Stopped); };
                break;

            case Starting:
                logger.info("Starting");
                playPauseButton.setEnabled(false);
                stopButton.setEnabled(true);
                transport.start();
                break;

            case Playing:
                logger.info("Playing");
                if (onPlaying != nullptr) {
                    onPlaying();
                }
                playPauseButton.setButtonText("Pause");
                playPauseButton.setEnabled(true);
                playPauseButton.onClick = [this]() { transportStateChanged(Pausing); };
                stopButton.setEnabled(true);
                stopButton.onClick = [this]() { transportStateChanged(Stopping); };
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

void AudioSourceComponent::setMinimumBufferSize() {
    auto* currentDevice = deviceManager.getCurrentAudioDevice();
    if (currentDevice == nullptr) {
        logger.debug("No current audio device - cannot set buffer size");
        return;
    }

    const auto availableBufferSizes = currentDevice->getAvailableBufferSizes();
    if (availableBufferSizes.isEmpty()) {
        logger.debug("No available buffer sizes reported by device");
        return;
    }

    int minBufferSize =
        *std::min_element(availableBufferSizes.begin(), availableBufferSizes.end());

    auto currentSetup = deviceManager.getAudioDeviceSetup();
    if (currentSetup.bufferSize == minBufferSize) {
        logger.debug("Buffer size already at minimum: " + std::to_string(minBufferSize));
        return;
    }

    currentSetup.bufferSize = minBufferSize;
    const std::string errorMessage =
        deviceManager.setAudioDeviceSetup(currentSetup, true).toStdString();

    if (!errorMessage.empty()) {
        logger.error("Error setting minimum buffer size: " + errorMessage);
        return;
    }

    logger.info("Buffer size set to minimum: " + std::to_string(minBufferSize));
}
} // namespace AudioApp