#pragma once

#include "CommonHeader.h"
#include "Logger.h"

namespace AudioApp
{
class AudioSourceComponent : public juce::AudioSource, public juce::Component, public juce::ChangeListener
    {
    public:
        explicit AudioSourceComponent(juce::AudioDeviceManager& deviceManager, Logger& logger);

        void paint(Graphics& g) override;
        void resized() override;

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

        // get frame values from last block of audio processed
        double* getFrameValues();

    private:
        std::vector<double> frameValues2;
        Logger& logger;

        juce::AudioDeviceManager& deviceManager;

        void changeListenerCallback (juce::ChangeBroadcaster *source) override;

        juce::AudioDeviceSelectorComponent selector {
            deviceManager, 2, 2, 2, 2, false, false, true, false};

        // File play
        enum TransportState
        {
            Stopped,
            Starting,
            Stopping,
            Playing
        };

        TransportState state;

        void openButtonClicked();
        std::unique_ptr<juce::FileChooser> fileChooser_;
        void chooserClosed(const juce::FileChooser& chooser);
        void playButtonClicked();
        void stopButtonClicked();
        void transportStateChanged(TransportState newState);

        juce::TextButton openButton;
        juce::TextButton playButton;
        juce::TextButton stopButton;

        juce::AudioFormatManager formatManager;
        std::unique_ptr<juce::AudioFormatReaderSource> playSource;
        juce::AudioTransportSource transport;
    };
}
