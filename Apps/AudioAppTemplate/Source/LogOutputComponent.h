#pragma once

#include "CommonHeader.h"
#include <cmath>

namespace AudioApp
{
    class LogOutputComponent : public juce::Component
    {
    public:
        LogOutputComponent();

        void paint(Graphics& g) override;
        void resized() override;
        void log(const String &message);

    private:
        juce::Label message;
        std::vector<std::string> logMessages;
    };
}
