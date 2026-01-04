#pragma once

#include "JuceHeader.h"
#include "../Components/LabelledSlider.h"
#include "Analyser.h"
#include "AnalyserSettings.h"
#include "ValueHistoryComponent.h"
#include "../AudioSourceComponent.h"
#include "../Logger/Logger.h"
#include "../Logger/StdoutLogger.h"
#include "../OSCComponent.h"

namespace Loudness {

/**
 * @brief UI component that wraps the Loudness::Analyser and provides visualization.
 *
 * This component serves as the integration point between the audio system and the
 * loudness processing chain. It:
 * - Receives audio samples from MainComponent::getNextAudioBlock()
 * - Feeds samples to the Analyser for processing
 * - Displays real-time loudness visualization via ValueHistoryComponent
 * - Provides UI controls for adjusting processing parameters
 * - Invokes a callback to send loudness values externally (e.g., via OSC)
 *
 * ## Integration
 *
 * ```
 * MainComponent::getNextAudioBlock()
 *       ↓
 * AnalyserComponent::pushNextSampleIntoFifo()
 *       ↓
 * Loudness::Analyser (processing chain)
 *       ↓
 * ├── ValueHistoryComponent (visualization)
 * └── onLoudness callback → OSC output
 * ```
 *
 * ## Default Processing Parameters
 *
 * - Frequency Band: 2-13% of Nyquist (focused on lower frequencies)
 * - Moving Average Window: 2 samples
 * - Decay Coefficient: 0.8
 *
 * FFT processing is event-driven (triggered when FIFO buffer fills) for minimum latency.
 */
class AnalyserComponent : public juce::Component {
public:
    /**
     * @param onLoudnessCallback Called with each new loudness value [0.0-1.0].
     *                           Return true if the value was successfully sent.
     */
    explicit AnalyserComponent(std::function<bool(float)> onLoudnessCallback)
        : onLoudness(std::move(onLoudnessCallback)) {
        addAndMakeVisible(&valueHistoryComp);
        addAndMakeVisible(loudnessAnalyserSettings);
    }

    //==============================================================================
    // Component functions

    void resized() override {
        auto bounds = getLocalBounds();

        valueHistoryComp.setBounds(bounds);
        loudnessAnalyserSettings.setBounds(
            bounds.getProportion(juce::Rectangle(0.f, 0.f, 0.6f, 1.f)));
    }

    //==============================================================================
    // AudioAppComponent functions

    void pushNextSampleIntoFifo(float sample) noexcept {
        loudnessAnalyser.pushNextSampleIntoFifo(sample);
    }

    /** Update sample rate for frequency display calculations */
    void setSampleRate(double sampleRate) {
        loudnessAnalyserSettings.setSampleRate(sampleRate);
    }

    //==============================================================================
    void paint(Graphics& g) override { g.fillAll(Colours::black); }

private:
    std::function<bool(float)> onLoudness;
    float lastLevelSent = -10.f; // set to strange value to start off with

    Loudness::Analyser loudnessAnalyser {
        [safeThis = juce::Component::SafePointer<AnalyserComponent>(this)](float level) {
            // This callback is invoked from the processing thread (not the message thread).
            // JUCE UI components must only be accessed from the message thread.
            //
            // MessageManager::callAsync() posts a lambda to the message thread's event queue.
            // The lambda will be executed on the next message loop iteration (~1ms latency).
            // This is similar to JavaScript's setTimeout(fn, 0) or Qt's QMetaObject::invokeMethod.
            //
            // We capture 'level' by value because it's a simple float - safe to copy.
            // We use SafePointer instead of raw 'this' to handle the case where the component
            // is destroyed while a lambda is still queued. SafePointer becomes null when the
            // component is deleted, preventing use-after-free crashes.
            juce::MessageManager::callAsync([safeThis, level]() {
                if (safeThis == nullptr) {
                    return; // Component was destroyed, bail out
                }
                safeThis->valueHistoryComp.addLevel(level);

                // Optionally display observed input range (for debugging/feedback)
                if (safeThis->loudnessAnalyser.rangeAdapter.isEnabled()) {
                    safeThis->loudnessAnalyserSettings.updateObservedRangeDisplay(
                        safeThis->loudnessAnalyser.rangeAdapter.getObservedMin(),
                        safeThis->loudnessAnalyser.rangeAdapter.getObservedMax());
                }

                if (level != safeThis->lastLevelSent) {
                    if (safeThis->onLoudness && safeThis->onLoudness(level))
                        safeThis->lastLevelSent = level;
                }
            });
        },
        initialProcessingBandLow,
        initialProcessingBandHigh,
        movingAverageInitialWindow,
        initialDecayExponent};
    AnalyserSettings loudnessAnalyserSettings {loudnessAnalyser,
                                               initialProcessingBandLow,
                                               initialProcessingBandHigh,
                                               movingAverageInitialWindow,
                                               initialDecayExponent};

    ValueHistoryComponent valueHistoryComp;

    static constexpr float initialDecayExponent = 0.8f;
    static constexpr int movingAverageInitialWindow = 2;
    static constexpr double initialProcessingBandLow = 0.02;
    static constexpr double initialProcessingBandHigh = 0.13;
};
} // namespace Loudness
