#pragma once

#include <cmath>
#include <juce_audio_utils/juce_audio_utils.h>
#include "AudioSourceComponent.h"
#include "FlashBox.h"
#include "LogOutputComponent.h"
#include "Loudness/AnalyserComponent.h"
#include "OSCComponent.h"
#include "TempoAnalyserComponent.h"
#include "TempoSynthesizerComponent.h"
#include "AvvaOSCSender.h"

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

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    LogOutputComponent logger;
    OSCComponent oscComponent { logger };
    AvvaOSCSender oscSender { oscComponent };
    AudioSourceComponent audioSource { deviceManager, logger };

    TempoAnalyserComponent tempoAnalyser;
    FlashBox tempoAnalyserFlash;
    TempoSynthesizerComponent tempoSynthesizer { logger };
    FlashBox tempoSynthesizerFlash;

    Loudness::AnalyserComponent analyserComponent { oscSender };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
}
