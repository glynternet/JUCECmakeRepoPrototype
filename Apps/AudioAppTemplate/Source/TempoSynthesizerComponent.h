#pragma once

#include <cmath>
#include "CommonHeader.h"
#include "../Libs/BTrack/BTrack.h"
#include "Repeat.h"

namespace AudioApp
{
    class TempoSynthesizerComponent : public juce::Component, juce::Timer
    {
    public:
        TempoSynthesizerComponent();

        void paint(Graphics& g) override;
        void resized() override;
        void timerCallback() override;

        void beat(long long period);

    private:
        uint8_t currentBeat = 0;
        juce::int64 lastTime = juce::Time::currentTimeMillis();
        double diffEwma = 0;

        juce::Label label;
        Colour colour = juce::Colours::grey;
        std::string content;
        std::atomic<bool> dirty;

        void flash(int duration);
        void updateLabel(uint8_t beat);
        void updateLabelColour(juce::Colour newColour);

        static double ewma(double current, double nextValue, double alpha);
    };
}
