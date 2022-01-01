#pragma once

#include <cmath>
#include "CommonHeader.h"
#include "../Libs/BTrack/BTrack.h"

namespace AudioApp
{
    class TempoSynthesizerComponent : public juce::Component, juce::Timer
    {
    public:
        TempoSynthesizerComponent();

        void paint(Graphics& g) override;
        void resized() override;
        void timerCallback() override;

        void beat(double period);

    private:

        uint64_t beats = 0;
        uint8_t currentBeat = 0;
        juce::int64 lastTime = juce::Time::currentTimeMillis();

        juce::Label label;
        Colour colour = juce::Colours::grey;
        std::string content;
        std::atomic<bool> dirty;

        void updateLabel(uint8_t beat);

        static void repeatFunc(int interval, int count, const std::function<void()>& call);
    };
}
