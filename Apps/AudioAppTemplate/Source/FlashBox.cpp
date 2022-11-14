//
// Created by glynh on 14/11/2022.
//

#include "FlashBox.h"

namespace AudioApp {
    static const int fadeIncrements = 16;

    FlashBox::FlashBox() {
        juce::Timer::startTimerHz(60);
    }

    void FlashBox::flash(float duration) {
        Repeat::repeatFunc(duration/fadeIncrements, fadeIncrements, [this](int i){
            this->updateColour(juce::Colours::white.interpolatedWith(juce::Colours::grey, (float) i / float(fadeIncrements-1)));
        });
    };

    void FlashBox::paint(Graphics &g) {
        g.fillAll(colour);
    }

    void FlashBox::timerCallback() {
        if (dirty.exchange(false)) {
            repaint();
        }
    }

    void FlashBox::updateColour(juce::Colour newColour) {
        colour = newColour;
        dirty = true;
    }
}