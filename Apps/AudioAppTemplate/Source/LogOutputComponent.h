#pragma once

#include "CommonHeader.h"
#include <cmath>

namespace AudioApp
{
    class LogOutputComponent : public juce::Component, juce::Timer
    {
    public:
        LogOutputComponent();

        void paint(Graphics& g) override;
        void resized() override;
        void log(const String &message);
        void timerCallback() override;

    private:
        juce::Label label;
        std::vector<std::string> logMessages;
        std::string content;
        std::atomic<bool> dirty;
    };
}
