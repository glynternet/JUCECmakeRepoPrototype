#pragma once

#include "CommonHeader.h"
#include "../Libs/BTrack/BTrack.h"
#include <cmath>

namespace AudioApp
{
class MainComponent : public juce::AudioAppComponent
{

    static const int btrackFrameSize = 512;
    static const int btrackHopSize = 256;

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

    BTrack b { btrackHopSize, btrackFrameSize };
    int count = 0;
    // hop size and frame size. For easiest maintenance we want
    // frame size to be a full buffer size from the audio settings
//    b = BTrack(btrackHopSize, btrackFrameSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace AudioApp
