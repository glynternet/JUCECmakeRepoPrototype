#pragma once

#include <atomic>
#include <functional>
#include <juce_events/juce_events.h>
#include "../Math.h"

namespace Beat {

// BeatSynthesizer uses a phase accumulator approach to synthesize beats
// at multiples or divisions of the detected beat rate.
class Synthesizer : public juce::HighResolutionTimer {
public:
    Synthesizer();
    ~Synthesizer() override;

    // Called when a beat is detected, with the period since the last beat
    void beat(double periodMs);

    // Set the multiplier index (0 = most downsampling, higher = more upsampling)
    void setMultipleIndex(int index);

    // Get current multiplier index
    int getMultipleIndex() const { return multipleIndex.load(); }

    // Get the next multiplier index (pending change)
    int getNextMultipleIndex() const { return nextMultipleIndex.load(); }

    // Set the next multiplier index (will take effect on next detected beat)
    void setNextMultipleIndex(int index);

    // Callback fired whenever a synthesized beat occurs
    std::function<void(double)> onSynthesizedBeat;

    static constexpr int negativeMultipleCount = 3;
    static constexpr int positiveMultipleCount = 6;
    static constexpr int totalMultipleCount = positiveMultipleCount + 1 + negativeMultipleCount;

private:
    void hiResTimerCallback() override;

    int calculateMultipleFromIndex(int index) const;

    std::atomic<double> periodMs {500.0};
    std::atomic<double> phase {0.0};
    std::atomic<int> multiple {1};
    std::atomic<int> multipleIndex {negativeMultipleCount}; // Start at 1:1
    std::atomic<int> nextMultipleIndex {negativeMultipleCount};
    std::atomic<bool> isUpsampling {false};

    // For downsampling: count input beats
    std::atomic<uint32_t> inputBeatCount {0};
    std::atomic<uint32_t> lastOutputBeatCount {0};

    double lastTimeMs {0.0};
};

} // namespace Beat
