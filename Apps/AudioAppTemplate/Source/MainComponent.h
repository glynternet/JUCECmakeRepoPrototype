#pragma once

#include "CommonHeader.h"
#include "../Libs/Btrack/BTrack.h"
#include <cmath>

namespace AudioApp
{
class MainComponent : public juce::AudioAppComponent
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

    // these need to be set above where we initialise b
    int btrackFrameSize = 512;
    int btrackHopSize = 256;
    BTrack b { btrackHopSize, btrackFrameSize };

    juce::String tempo = "detecting...";
    juce::Label tempoLabel;
    bool paintBeatDetectionBright = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

}
