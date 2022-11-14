//
// Created by glynh on 14/11/2022.
//

#ifndef JUCECMAKEREPO_FLASHBOX_H
#define JUCECMAKEREPO_FLASHBOX_H

#include "CommonHeader.h"
#include "Repeat.h"

namespace AudioApp {
    class FlashBox : public juce::Component, juce::Timer {
    public:
        explicit FlashBox();

        void flash(float duration);
        void paint(Graphics& g);
        void timerCallback() override;
    private:
        std::atomic<bool> dirty{};
        Colour colour = juce::Colours::grey;
        void updateColour(juce::Colour newColour);
    };
}

#endif //JUCECMAKEREPO_FLASHBOX_H

