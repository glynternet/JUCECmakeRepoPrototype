#include "AnalyserComponent.h"
#include <juce_core/juce_core.h>

namespace Beat {
    AnalyserComponent::AnalyserComponent() {
        lastTime = juce::Time::getMillisecondCounterHiRes();
    }

    void AnalyserComponent::processAudioFrame(double *frame) {
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

    void AnalyserComponent::updateSamplePerBlockExpected(int samplePerBlockExpected) {
        btrack.updateHopAndFrameSize(samplePerBlockExpected / 2, samplePerBlockExpected);
    }
}