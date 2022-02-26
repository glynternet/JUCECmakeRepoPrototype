#pragma once

#include <cmath>
#include "CommonHeader.h"
#include "../Libs/BTrack/BTrack.h"
#include "Repeat.h"

namespace AudioApp
{
    class TempoAnalyserComponent : public juce::Component, juce::Timer
    {
    public:
        TempoAnalyserComponent();

        void paint(Graphics& g) override;
        void resized() override;
        void timerCallback() override;

        void processAudioFrame(double *frame);
        void updateSamplePerBlockExpected(int samplePerBlockExpected);

        // onBeat is run whenever a new beat is encountered by btrack and
        // receives the duration since last detected beat as a parameter.
        std::function<void(double)> onBeat;

    private:
        void flash(float duration);

        // these need to be set above where we initialise btrack
        int btrackFrameSize = 512;
        int btrackHopSize = 256;
        BTrack btrack {btrackHopSize, btrackFrameSize };

        // cannot initialise here as juce::Time::getMillisecondCounterHiRes() returns 0 at this point
        double lastTime;

        void setColour(Colour newColour);
        Colour colour = juce::Colours::grey;
        std::atomic<bool> dirty;
    };
}
