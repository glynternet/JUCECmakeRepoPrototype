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
        // TODO: probably a good idea to put some locking on here as I'm not really sure how safe this is
        std::vector<std::string> logMessages;
        std::string content;
        std::atomic<bool> dirty;
    };
}
