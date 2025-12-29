#pragma once

#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace AudioApp {
    class FlashBox : public juce::Component, juce::Timer {
    public:
        explicit FlashBox();

        void flash(float duration);
        void paint(juce::Graphics& g);
        void timerCallback() override;
    private:
        double flashStart = 0.0;
        float flashDuration = 0.0F;
        float brightness = 0.0F;

        std::atomic<bool> dirty{};
        juce::Colour colour = juce::Colours::grey;
    };
}

