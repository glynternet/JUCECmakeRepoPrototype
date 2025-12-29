#include "Analyser.h"
#include <juce_core/juce_core.h>

namespace Beat {
    Analyser::Analyser() {
        lastTime = juce::Time::getMillisecondCounterHiRes();
    }

    void Analyser::processAudioFrame(const std::vector<double>& frame) {
        if (frame.empty()) return;
        // BTrack API takes non-const pointer but doesn't modify the data
        btrack.processAudioFrame(const_cast<double*>(frame.data()));
        if (btrack.beatDueInCurrentFrame()) {
            auto current = juce::Time::getMillisecondCounterHiRes();
            auto diff = current - lastTime;
            lastTime = current;
            if (onBeat != nullptr) {
                onBeat(diff);
            }
        }
    }

    void Analyser::updateSamplePerBlockExpected(int samplePerBlockExpected) {
        btrack.updateHopAndFrameSize(samplePerBlockExpected / 2, samplePerBlockExpected);
    }
}