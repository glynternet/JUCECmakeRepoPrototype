#pragma once

#include "CommonHeader.h"
#include "../Libs/BTrack/BTrack.h"
#include <cmath>

namespace AudioApp
{
class MainComponent : public juce::AudioAppComponent,
                      public juce::ChangeListener
{
public:
    MainComponent();
    ~MainComponent();

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void paint(Graphics&) override;
    void resized() override;

private:
    juce::AudioDeviceSelectorComponent selector {
        deviceManager, 2, 2, 2, 2, false, false, true, false};
    WhiteNoise::Oscillator noise;

    juce::Label message;

    // File play
    enum TransportState
    {
        Stopped,
        Starting,
        Stopping,
        Playing
    };

    TransportState state;

    std::unique_ptr<juce::FileChooser> fileChooser_;

    void openButtonClicked();
    void chooserClosed(const juce::FileChooser& chooser);

    void playButtonClicked();
    void stopButtonClicked();
    void transportStateChanged(TransportState newState);
    void changeListenerCallback (juce::ChangeBroadcaster *source) override;

    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> playSource;
    juce::AudioTransportSource transport;

    // Tempo detection
    // these need to be set above where we initialise b
    int btrackFrameSize = 512;
    int btrackHopSize = 256;
    BTrack b { btrackHopSize, btrackFrameSize };

    int beats = 0;
    juce::Label tempoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

}
