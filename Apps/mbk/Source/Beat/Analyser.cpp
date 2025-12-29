#include "Analyser.h"
#include <juce_core/juce_core.h>

namespace Beat {
Analyser::Analyser()
    : lastTime(juce::Time::getMillisecondCounterHiRes()) {
}

void Analyser::processAudioFrame(const std::vector<double>& frame) {
    if (frame.empty()) {
        return;
    }
    // BTrack API takes non-const pointer but doesn't modify the data
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
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

void Analyser::setSampleRate(double sampleRate) {
    btrack.setSampleRate(sampleRate);
}
} // namespace Beat