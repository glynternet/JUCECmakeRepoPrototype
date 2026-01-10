#pragma once

#include "AudioSourceComponent.h"
#include "Components/FlashBox.h"
#include "Components/LogOutputComponent.h"
#include "Loudness/AnalyserComponent.h"
#include "Logger/MultiLogger.h"
#include "OSCComponent.h"
#include "Beat/Analyser.h"
#include "Beat/Synthesizer.h"
#include "Beat/SynthesizerComponent.h"
#include "AvvaOSCSender.h"

namespace AudioApp {
class MainComponent : public juce::AudioAppComponent {
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

    juce::Label versionLabel {"versionLabel", "version: v2.1.0"};
    OSCComponent oscComponent {logger};
    AvvaOSCSender oscSender {oscComponent};
    AudioSourceComponent audioSource {deviceManager, logger};

    Beat::Analyser tempoAnalyser;
    FlashBox tempoAnalyserFlash;
    Beat::Synthesizer tempoSynthesizer;
    Beat::SynthesizerComponent tempoSynthesizerComponent {tempoSynthesizer};
    FlashBox tempoSynthesizerFlash;

    Loudness::AnalyserComponent analyserComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace AudioApp
