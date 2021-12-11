#pragma once

#include <cmath>
#include "CommonHeader.h"
#include "AudioSourceComponent.h"
#include "LogOutputComponent.h"
#include "OSCComponent.h"
#include "TempoAnalyserComponent.h"

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
    LogOutputComponent logger;
    AudioSourceComponent audioSource { deviceManager, logger };
    TempoAnalyserComponent tempoAnalyser;
    OSCComponent oscComponent { logger };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
}
