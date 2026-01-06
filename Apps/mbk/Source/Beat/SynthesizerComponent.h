#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Synthesizer.h"

namespace Beat {

// UI component for controlling a beat Synthesizer.
// Displays buttons to select beat multiplication/division rate.
class SynthesizerComponent
    : public juce::Component
    , private juce::Timer {
public:
    explicit SynthesizerComponent(Synthesizer& synth);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateButtonStates();

    Synthesizer& synthesizer;

    std::atomic<bool> dirty {true};

    juce::ShapeButton up {"up",
                          juce::Colours::lightgrey,
                          juce::Colours::lightgrey,
                          juce::Colours::lightgrey};
    juce::ShapeButton down {"down",
                            juce::Colours::lightgrey,
                            juce::Colours::lightgrey,
                            juce::Colours::lightgrey};

    std::array<juce::ShapeButton, Synthesizer::totalMultipleCount> multipleButtons {{
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
    }};

    int lastMultipleIndex {-1};
    int lastNextMultipleIndex {-1};
};

} // namespace Beat
