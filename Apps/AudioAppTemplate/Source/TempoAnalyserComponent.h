#pragma once

#include <cmath>
#include "CommonHeader.h"
#include "../Libs/BTrack/BTrack.h"

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
        void beat();

        // these need to be set above where we initialise btrack
        int btrackFrameSize = 512;
        int btrackHopSize = 256;
        BTrack btrack {btrackHopSize, btrackFrameSize };

        uint64_t beats = 0;
        uint8_t currentBeat = 0;
        juce::int64 lastTime = juce::Time::currentTimeMillis();
        double diffEwma = 0;

        juce::Label label;
        Colour colour = juce::Colours::grey;
        std::string content;
        std::atomic<bool> dirty;

        static void repeatFunc(int interval, int count, const std::function<void()>& call);
        static double ewma(double current, double nextValue, double alpha);

        void setLabelColour(Colour newColout);
        void setLabelText(double btrackTempo, double manualCalcTempo, double movingAverageTempo);
    };
}
