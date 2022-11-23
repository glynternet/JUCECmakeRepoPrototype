#pragma once

#include "Logger.h"
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace AudioApp
{
class AudioSourceComponent : public juce::AudioSource, public juce::Component, public juce::ChangeListener
    {
    public:
        explicit AudioSourceComponent(juce::AudioDeviceManager& deviceManager, Logger& logger);

        void paint(juce::Graphics& g) override;
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

        // File transport
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
        juce::AudioFormatManager formatManager;
        void chooserClosed(const juce::FileChooser& chooser);
        void transportStateChanged(TransportState newState);
        juce::TextButton openButton;
        juce::TextButton playButton;
        juce::TextButton stopButton;
        std::unique_ptr<juce::AudioFormatReaderSource> playSource;
        juce::AudioTransportSource transport;

        // Source selection
        bool filePlayerEnabled = false;
        juce::ToggleButton sourceToggle {"enable file player"} ;
    };
}
