//
// Created by glynh on 14/11/2022.
//

#ifndef JUCECMAKEREPO_FLASHBOX_H
#define JUCECMAKEREPO_FLASHBOX_H

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
        double flashStart;
        float flashDuration;
        float brightness;

        std::atomic<bool> dirty{};
        juce::Colour colour = juce::Colours::grey;
    };
}

#endif //JUCECMAKEREPO_FLASHBOX_H

