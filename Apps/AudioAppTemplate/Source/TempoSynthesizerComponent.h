#pragma once

#include <cmath>
#include "CommonHeader.h"
#include "../Libs/BTrack/BTrack.h"
#include "Repeat.h"
#include "LogOutputComponent.h"

namespace AudioApp
{
    class TempoSynthesizerComponent : public juce::Component, juce::HighResolutionTimer, juce::Timer {
    public:
        explicit TempoSynthesizerComponent(Logger& logger);

        void paint(Graphics& g) override;
        void resized() override;
        void timerCallback() override;
        void hiResTimerCallback() override;

        void beat(long long period);
    private:
        Logger& logger;

        // max uint8 value so that next beat makes it start on 0
        uint8_t currentBeat = 255;
        juce::int64 lastTime = juce::Time::currentTimeMillis();
        double diffEwma = 0;

        juce::Label label;
        Colour colour = juce::Colours::grey;
        std::string content;
        std::atomic<bool> dirty;

        void flash(float duration);
        void updateBeat(uint8_t beat);
        void updateLabelColour(juce::Colour newColour);

        static double ewma(double current, double nextValue, double alpha);

        struct scheduledBeat {
            __int64_t millis;
            uint8_t beat;
        };

        // deffo put locking on here if we're using high res scheduler with access from everywhere
        std::list<scheduledBeat> scheduledBeats;
    };
}
