#include "TempoAnalyserComponent.h"
#include <juce_core/juce_core.h>

namespace AudioApp {
    TempoAnalyserComponent::TempoAnalyserComponent() {
        lastTime = juce::Time::getMillisecondCounterHiRes();
    }

    void TempoAnalyserComponent::processAudioFrame(double *frame) {
        if (frame == nullptr) return;
        btrack.processAudioFrame(frame);
        if (btrack.beatDueInCurrentFrame()) {
            auto current = juce::Time::getMillisecondCounterHiRes();
            auto diff = current - lastTime;
            lastTime = current;
            if (onBeat != nullptr) {
                onBeat(diff);
            }
        }
    }

    void TempoAnalyserComponent::updateSamplePerBlockExpected(int samplePerBlockExpected) {
        btrack.updateHopAndFrameSize(samplePerBlockExpected / 2, samplePerBlockExpected);
    }
}