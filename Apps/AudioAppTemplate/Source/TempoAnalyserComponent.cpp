#include "TempoAnalyserComponent.h"

namespace AudioApp
{
    TempoAnalyserComponent::TempoAnalyserComponent() {
        lastTime = juce::Time::getMillisecondCounterHiRes();
    }

    void TempoAnalyserComponent::processAudioFrame (double* frame) {
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

    void TempoAnalyserComponent::updateSamplePerBlockExpected(int samplePerBlockExpected){
        btrack.updateHopAndFrameSize(samplePerBlockExpected / 2, samplePerBlockExpected);
    }
}