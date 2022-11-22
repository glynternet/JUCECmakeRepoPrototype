//
// Created by glynh on 14/11/2022.
//

#include "FlashBox.h"

namespace AudioApp {
    FlashBox::FlashBox() {
        juce::Timer::startTimerHz(60);
    }

    void FlashBox::flash(float duration) {
        flashStart = juce::Time::getMillisecondCounterHiRes();
        flashDuration = duration;
        brightness = 1;
    };

    void FlashBox::paint(Graphics &g) {
        g.fillAll(colour);
    }

    void FlashBox::timerCallback() {
        // if brightness is 0, there is no flash in progress, return
        if (brightness == 0) {
            return;
        }

        auto now = juce::Time::getMillisecondCounterHiRes();
        auto timeSinceFlash = now - flashStart;
        if (timeSinceFlash > flashDuration) {
            brightness = 0;
            return;
        }

        brightness = 1.0f - ((float)timeSinceFlash / flashDuration);
        colour = juce::Colours::grey.interpolatedWith(juce::Colours::white, brightness);
        repaint();
    }
}