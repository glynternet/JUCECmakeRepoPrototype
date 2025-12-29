#pragma once

#include <cmath>
#include <functional>
#include <vector>
#include "../../Libs/BTrack/BTrack.h"

namespace Beat {
class Analyser {
public:
    Analyser();

    void processAudioFrame(const std::vector<double>& frame);
    void updateSamplePerBlockExpected(int samplePerBlockExpected);
    void setSampleRate(double sampleRate);

    // onBeat is run whenever a new beat is encountered by btrack and
    // receives the duration since last detected beat as a parameter.
    std::function<void(double)> onBeat;

private:
    // these need to be set above where we initialise btrack
    int btrackFrameSize = 512;
    int btrackHopSize = 256;
    BTrack btrack {btrackHopSize, btrackFrameSize};

    // cannot initialise here as juce::Time::getMillisecondCounterHiRes() returns 0 at this point
    double lastTime;
};
} // namespace Beat
