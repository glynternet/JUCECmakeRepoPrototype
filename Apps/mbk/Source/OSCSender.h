#pragma once

#include <cmath>
#include <juce_osc/juce_osc.h>

namespace AudioApp {
    class OSCSender {
    public:
        virtual bool send(const juce::OSCMessage &message) = 0;
    };
}
