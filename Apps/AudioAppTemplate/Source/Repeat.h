#pragma once

#include "CommonHeader.h"
#include <cmath>

namespace AudioApp
{
    class Repeat
    {
    public:
        static void repeatFunc(int interval, int count, const std::function<void()>& call) {
            // TODO: use a single timer here instead?
            // Make first call without overhead of using callAfterDelay.
            call();
            for (int i = 1; i < count; ++i) {
                juce::Timer::callAfterDelay((i)*interval, call);
            }
        }

        static void repeatFunc(int interval, int count, const std::function<void(int index)>& call) {
            // TODO: use a single timer here instead?
            // Make first call without overhead of using callAfterDelay.
            call(0);
            for (int i = 1; i < count; ++i) {
                juce::Timer::callAfterDelay((i)*interval, [call, i]{
                    call(i);
                });
            }
        }
    };
}
