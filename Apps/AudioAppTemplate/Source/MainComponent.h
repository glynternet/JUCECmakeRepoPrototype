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
    void log(const String &message);

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

    // Tempo detection
    // these need to be set above where we initialise b
    int btrackFrameSize = 512;
    int btrackHopSize = 256;
    BTrack b { btrackHopSize, btrackFrameSize };

    uint64_t beats = 0;
    uint8_t beat = 0;
    juce::Label tempoLabel;
    juce::int64 lastTime = juce::Time::currentTimeMillis();
    double diffEwma = 0;

    void repeatFunc(int interval, int count, std::function<void()> call);

    double ewma(double current, double nextValue, double alpha) const;
    juce::TextButton connectOSCButton;
    // Probably worth taking a look at the AVVAOSCSender class from the legacy repo
    juce::OSCSender sender;

    bool senderConnected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
}
