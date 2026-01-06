#include "Synthesizer.h"

namespace Beat {

Synthesizer::Synthesizer() {
    lastTimeMs = juce::Time::getMillisecondCounterHiRes();
    startTimer(1);
}

Synthesizer::~Synthesizer() {
    stopTimer();
}

void Synthesizer::hiResTimerCallback() {
    if (!isUpsampling.load()) {
        return;
    }

    double now = juce::Time::getMillisecondCounterHiRes();
    double elapsed = now - lastTimeMs;
    lastTimeMs = now;

    double currentPeriod = periodMs.load();
    int currentMultiple = multiple.load();
    double synthPeriod = currentPeriod / static_cast<double>(currentMultiple);

    double currentPhase = phase.load();
    currentPhase += elapsed / synthPeriod;

    while (currentPhase >= 1.0) {
        currentPhase -= 1.0;
        if (onSynthesizedBeat) {
            onSynthesizedBeat(synthPeriod);
        }
    }

    phase.store(currentPhase);
}

void Synthesizer::beat(double period) {
    inputBeatCount.fetch_add(1);

    // Initialize or update period with EWMA
    double currentPeriod = periodMs.load();
    if (currentPeriod == 0.0) {
        periodMs.store(period);
    } else {
        periodMs.store(ewma(currentPeriod, period, 0.5));
    }

    // Apply pending multiplier change
    int next = nextMultipleIndex.load();
    if (multipleIndex.load() != next) {
        setMultipleIndex(next);
    }

    int relativeIndex = multipleIndex.load() - negativeMultipleCount;

    if (relativeIndex > 0) {
        // Upsampling: phase accumulator handles it
        // Reset phase on detected beat to stay locked
        phase.store(0.0);
        lastTimeMs = juce::Time::getMillisecondCounterHiRes();
        isUpsampling.store(true);

        // Fire first beat immediately
        double synthPeriod = periodMs.load() / static_cast<double>(multiple.load());
        if (onSynthesizedBeat) {
            onSynthesizedBeat(synthPeriod);
        }

    } else if (relativeIndex == 0) {
        // 1:1 passthrough
        isUpsampling.store(false);
        if (onSynthesizedBeat) {
            onSynthesizedBeat(periodMs.load());
        }

    } else {
        // Downsampling: only fire every Nth beat
        isUpsampling.store(false);
        int currentMultiple = multiple.load();
        if (inputBeatCount.load() % currentMultiple == 0) {
            double synthPeriod = periodMs.load() * static_cast<double>(currentMultiple);
            if (onSynthesizedBeat) {
                onSynthesizedBeat(synthPeriod);
            }
        }
    }
}

void Synthesizer::setMultipleIndex(int index) {
    if (index < 0 || index >= totalMultipleCount) {
        return;
    }
    multipleIndex.store(index);
    multiple.store(calculateMultipleFromIndex(index));
}

void Synthesizer::setNextMultipleIndex(int index) {
    if (index < 0 || index >= totalMultipleCount) {
        return;
    }
    nextMultipleIndex.store(index);
}

int Synthesizer::calculateMultipleFromIndex(int index) const {
    int exponent = index - negativeMultipleCount;
    if (exponent < 0) {
        exponent = -exponent;
    }
    return ipow(2, exponent);
}

} // namespace Beat
