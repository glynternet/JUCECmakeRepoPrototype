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
        TempoSynthesizerComponent();

        void paint(Graphics& g) override;
        void resized() override;
        void timerCallback() override;
        void hiResTimerCallback() override;

        void beat(double period);
    private:
        // max uint8 value so that next beat makes it start on 0
        uint8_t currentBeat = 255;
        double diffEwma = 0;

        Colour colour = juce::Colours::grey;
        std::atomic<bool> dirty{};

        void flash(float duration);
        void updateBeat(uint8_t beat);
        void updateColour(juce::Colour newColour);

        int multiplierIndex = 2;
        int nextMultiplierIndex = 2;
        juce::ShapeButton up {"up", juce::Colours::lightgrey, juce::Colours::lightgrey, juce::Colours::lightgrey};
        juce::ShapeButton down {"down", juce::Colours::lightgrey, juce::Colours::lightgrey, juce::Colours::lightgrey};

        #define MULTIPLIER_COUNT 9
        int multiplierValues[MULTIPLIER_COUNT]{};
        juce::ShapeButton multiplierValueButtons[MULTIPLIER_COUNT]{
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
            {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
        };

        struct scheduledBeat {
            double millis;
            uint8_t beat;
        };

        void setMultiplierIndex(int m);
        void setNextMultiplierIndex(int m);

        // deffo put locking on here if we're using high res scheduler with access from everywhere
        std::list<scheduledBeat> scheduledBeats;
    };
}
