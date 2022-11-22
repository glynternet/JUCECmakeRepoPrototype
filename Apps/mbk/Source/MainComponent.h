#pragma once

#include <cmath>
#include "CommonHeader.h"
#include "AudioSourceComponent.h"
#include "FlashBox.h"
#include "LogOutputComponent.h"
#include "OSCComponent.h"
#include "TempoAnalyserComponent.h"
#include "TempoSynthesizerComponent.h"

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
    FlashBox tempoAnalyserFlash;
    TempoSynthesizerComponent tempoSynthesizer { logger };
    FlashBox tempoSynthesizerFlash;
    OSCComponent oscComponent { logger };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
}
