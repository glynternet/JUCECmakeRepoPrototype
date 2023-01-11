#pragma once

#include <cmath>
#include "../../Libs/BTrack/BTrack.h"
#include "../Components/LogOutputComponent.h"
#include "../Logger/Logger.h"

namespace Beat {
    class SynthesizerComponent
    : public juce::Component, juce::HighResolutionTimer, juce::Timer {
    public:
        explicit SynthesizerComponent(logger::Logger &logger);

        void paint(juce::Graphics &g) override;
        void resized() override;
        void timerCallback() override;
        void hiResTimerCallback() override;

        void beat(double period);

        // onSynthesizedBeat is run whenever a new beat is synthesized and
        // receives the duration since last synthesized beat as a parameter.
        std::function<void(double)> onSynthesizedBeat;
    private:
        logger::Logger &logger;

        uint32_t inputBeatCount = 0;

        double diffEwma = 0;

        std::atomic<bool> dirty{};

        void synthesizedBeat(double duration);

        // multiple is the number of beats on top of the input beat in which we want to synthesise beats for.
        int multiple = 1;
        int multipleIndex = 2;
        int nextMultipleIndex = 2;
        juce::ShapeButton up{"up", juce::Colours::lightgrey, juce::Colours::lightgrey, juce::Colours::lightgrey};
        juce::ShapeButton down{"down", juce::Colours::lightgrey, juce::Colours::lightgrey, juce::Colours::lightgrey};
#define NEGATIVE_MULTIPLE_COUNT 3
        // POSITIVE_MULTIPLE_COUNT cannot be higher than 8 because it is used as an exponent for a base of 2.
        // Where ipow(2, 8) is 256. For synthesis, this value has 1 taken off it and fits into a uint8,
        // which is what is used for the scheduledBeat creating loop.
        // Anything greater would overflow.
#define POSITIVE_MULTIPLE_COUNT 6
#define TOTAL_MULTIPLE_COUNT POSITIVE_MULTIPLE_COUNT+1+NEGATIVE_MULTIPLE_COUNT
        juce::ShapeButton multipleButtons[TOTAL_MULTIPLE_COUNT]{
                {"", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey},
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
        };

        void setMultipleFromIndex(int m);
        void setNextMultipleIndex(int m);

        // deffo put locking on here if we're using high res scheduler with access from everywhere
        std::list<scheduledBeat> scheduledBeats;
    };
}
