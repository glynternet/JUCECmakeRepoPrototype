#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

namespace AudioApp
{
    class Logger
    {
    public:
        virtual void debug(const juce::String &message) = 0;
        virtual void info(const juce::String &message) = 0;
        virtual void error(const juce::String &message) = 0;
    };
}
