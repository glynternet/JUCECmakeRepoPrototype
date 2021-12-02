#pragma once

#include <cmath>
#include "CommonHeader.h"
#include "../Libs/BTrack/BTrack.h"
#include "LogOutputComponent.h"
#include "TempoAnalyserComponent.h"

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

    LogOutputComponent logger;

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

    void changeListenerCallback (juce::ChangeBroadcaster *source) override;

    void connectOSCSender();
    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> playSource;
    juce::AudioTransportSource transport;

    void sendBeatMessage();
    TempoAnalyserComponent tempoAnalyser;

    juce::TextButton connectOSCButton;
    // Probably worth taking a look at the AVVAOSCSender class from the legacy repo
    juce::OSCSender sender;

    bool senderConnected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
}
