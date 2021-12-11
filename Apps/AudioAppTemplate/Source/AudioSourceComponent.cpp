#include "AudioSourceComponent.h"

namespace AudioApp
{
AudioSourceComponent::AudioSourceComponent(juce::AudioDeviceManager& deviceManager, Logger& logger)
    : deviceManager(deviceManager), logger(logger), state(Stopped), openButton("Open"), playButton("Play"), stopButton("Stop")
{
        openButton.onClick = [this] {  openButtonClicked(); };
        addAndMakeVisible(&openButton);

        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setEnabled(true);
        addAndMakeVisible(&playButton);

        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setEnabled(false);
        addAndMakeVisible(&stopButton);

        formatManager.registerBasicFormats();
        transport.addChangeListener(this);

        addAndMakeVisible(selector);
    }

    void AudioSourceComponent::paint(Graphics& g) {
    }

    void AudioSourceComponent::resized() {
        auto selectorBounds = getLocalBounds().withTrimmedBottom(50);
        selector.setBounds(selectorBounds);
        int buttonWidth = (getWidth() - 40) / 3;
        int buttonsY = selectorBounds.getBottom() + 10;
        openButton.setBounds(10, buttonsY, buttonWidth, 30);
        playButton.setBounds(20 + buttonWidth, buttonsY, buttonWidth, 30);
        stopButton.setBounds(30 + 2 * buttonWidth, buttonsY, buttonWidth, 30);
    }

    void AudioSourceComponent::changeListenerCallback (juce::ChangeBroadcaster *source)
    {
        logger.log("Change listened callback triggered");
        if (source == &transport)
        {
            if (transport.isPlaying())
            {
                transportStateChanged(Playing);
            }
            else
            {
                transportStateChanged(Stopped);
            }
        }
    }

    void AudioSourceComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
        logger.log("Preparing to play");
        transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void AudioSourceComponent::releaseResources() {
        // I THINK THIS IS NOT NEEDED BECUASE RESOURCES ARE RELEASED IN THE MAINCOMPONENT
    }

    void AudioSourceComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        // TODO(glynternet): some logic around handling file vs device input.
        transport.getNextAudioBlock(bufferToFill);

        if (bufferToFill.buffer->getNumChannels() == 0) {
            logger.log("No channels in buffer to fill");
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        const auto* inputData = bufferToFill.buffer->getReadPointer (0, bufferToFill.startSample);
        auto* outputData = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);

        // as prescribed in BTrack README: https://github.com/adamstark/BTrack
        // TODO(glynternet): is there a float version of BTrack so that we can avoid this conversion of float to double and avoid creating a vector?
        // TODO(glynternet): if the above todo is not possible, can we reuse this vector to avoid having to create a new one every audio block?
        std::vector<double> frameValues(bufferToFill.numSamples);

        for (auto i = 0; i < bufferToFill.numSamples; ++i) {
            frameValues[i] = inputData[i];

            // TODO(glynternet): some logic around handling file vs device input.
            //   If from file, the transport will have already written to the output portion of the buffer.
            outputData[i] = inputData[i];
        }

        frameValues2 = frameValues;
    }

    double* AudioSourceComponent::getFrameValues() {
        return frameValues2.data();
    }

    void AudioSourceComponent::openButtonClicked()
    {
        logger.log("Open button clicked");
        fileChooser_ = std::make_unique<juce::FileChooser> (("Choose a Patch to open..."),
                                                           juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                                           // TODO: MP3 doesn't seem to work on z30-a linux.
                                                           //   Does it work on any other machine OS combo?
                                                           "*.wav");

        fileChooser_->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this](const juce::FileChooser &fileChooser) {
                                      this->chooserClosed(fileChooser);
                                  });
    }

    void AudioSourceComponent::chooserClosed(const juce::FileChooser& chooser){
        juce::File file (chooser.getResult());

        logger.log("chooserClosed");

        juce::AudioFormatReader* reader = formatManager.createReaderFor(file);
        if (reader != nullptr)
        {
            //get the file ready to play
            std::unique_ptr<juce::AudioFormatReaderSource> tempSource (new juce::AudioFormatReaderSource (reader, true));

            transport.setSource(tempSource.get());
            transportStateChanged(Stopped);

            playSource.reset(tempSource.release());
        }
        logger.log("Reaader prepated");
    }

    void AudioSourceComponent::playButtonClicked()
    {
        logger.log("Play button clicked");
        transportStateChanged(Starting);
    }

    void AudioSourceComponent::stopButtonClicked()
    {
        logger.log("Stop button clicked");
        transportStateChanged(Stopping);
    }

    void AudioSourceComponent::transportStateChanged(TransportState newState)
    {
        if (newState != state)
        {
            state = newState;

            switch (state) {
                case Stopped:
                    playButton.setEnabled(true);
                    transport.setPosition(0.0);
                    break;

                case Playing:
                    playButton.setEnabled(true);
                    break;

                case Starting:
                    stopButton.setEnabled(true);
                    playButton.setEnabled(false);
                    transport.start();
                    break;

                case Stopping:
                    playButton.setEnabled(true);
                    stopButton.setEnabled(false);
                    transport.stop();
                    break;
            }
        }
    }
}