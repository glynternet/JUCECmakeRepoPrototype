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

        // multiple is the number of beats on top of the input beat in which we want to synthesise beats for.
        int multiple = 1;
        int multipleIndex = 2;
        int nextMultipleIndex = 2;
        juce::ShapeButton up {"up", juce::Colours::lightgrey, juce::Colours::lightgrey, juce::Colours::lightgrey};
        juce::ShapeButton down {"down", juce::Colours::lightgrey, juce::Colours::lightgrey, juce::Colours::lightgrey};
        // MULTIPLE_COUNT cannot be higher than 9 because the index of the multipleButtons is used as an exponent for a base of 2.
        // Where a MULTIPLE_COUNT of 9 gives a highest index of 8, so ipow(2, 8) is 256. For synthesis, this value has 1
        // taken off it and just fits into a uint8, which is what we store the current beat in.
        #define MULTIPLE_COUNT 9
        juce::ShapeButton multipleButtons[MULTIPLE_COUNT]{
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

        void setMultipleFromIndex(int m);
        void setNextMultipleIndex(int m);

        // deffo put locking on here if we're using high res scheduler with access from everywhere
        std::list<scheduledBeat> scheduledBeats;
    };
}
