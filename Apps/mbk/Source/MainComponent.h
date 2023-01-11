#pragma once

#include "AudioSourceComponent.h"
#include "Components/FlashBox.h"
#include "Components/LogOutputComponent.h"
#include "Loudness/AnalyserComponent.h"
#include "Logger/MultiLogger.h"
#include "OSCComponent.h"
#include "Beat/AnalyserComponent.h"
#include "Beat/SynthesizerComponent.h"
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
    logger::StdoutLogger stdoutLogger {true};
    LogOutputComponent uiLogger;
    logger::MultiLogger logger {{&stdoutLogger, &uiLogger}};

    OSCComponent oscComponent {logger};
    AvvaOSCSender oscSender {oscComponent};
    AudioSourceComponent audioSource {deviceManager, logger};

    Beat::AnalyserComponent tempoAnalyser;
    FlashBox tempoAnalyserFlash;
    Beat::SynthesizerComponent tempoSynthesizer {logger};
    FlashBox tempoSynthesizerFlash;

    Loudness::AnalyserComponent analyserComponent {oscSender};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace AudioApp
