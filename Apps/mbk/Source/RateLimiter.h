#pragma once

#include <juce_core/juce_core.h>

namespace AudioApp {

class RateLimiter {
public:
    explicit RateLimiter(juce::uint32 intervalMs)
        : interval(intervalMs) {}

    // Returns true if the action should be allowed (interval has passed)
    // Updates internal timestamp when returning true
    bool allowNow() {
        auto now = juce::Time::getMillisecondCounter();
        if (now - lastAllowed >= interval) {
            lastAllowed = now;
            return true;
        }
        return false;
    }

    // Reset the rate limiter so the next call to allowNow() returns true
    void reset() { lastAllowed = 0; }

private:
    juce::uint32 interval;
    juce::uint32 lastAllowed = 0;
};

} // namespace AudioApp
