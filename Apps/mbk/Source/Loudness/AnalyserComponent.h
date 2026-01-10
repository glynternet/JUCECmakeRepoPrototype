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
 * - Smoothing: 0.1 (light EWMA smoothing)
 * - Decay Coefficient: 0.8
 *
 * FFT processing is event-driven (triggered when FIFO buffer fills) for minimum latency.
 */
class AnalyserComponent : public juce::Component {
    static constexpr float initialDecayExponent = 0.8F;
    static constexpr float initialSmoothing = 0.4F;
    static constexpr double initialProcessingBandLow = 0.02;
    static constexpr double initialProcessingBandHigh = 0.30;
    static constexpr float initialRangeInMin = 0.1F;
    static constexpr float initialRangeInMax = 0.8F;
    static constexpr float initialOutputMin = 0.4F;
    static constexpr float initialOutputMax = 1.0F;
    static constexpr float initialAdaptationRate = 0.005F;

public:
    /**
     * @param onLoudnessCallback Called with each new loudness value [0.0-1.0].
     *                           Return true if the value was successfully sent.
     */
    explicit AnalyserComponent(std::function<bool(float)> onLoudnessCallback)
        : onLoudness(std::move(onLoudnessCallback)) {
        addAndMakeVisible(&valueHistoryComp);
        addAndMakeVisible(loudnessAnalyserSettings);

        // Wire up the index update callback to update the UI display
        loudnessAnalyser.onIndexUpdate =
            [safeThis = juce::Component::SafePointer<AnalyserComponent>(this)](
                float i10s, float i1m, float i5m, float range) {
                juce::MessageManager::callAsync([safeThis, i10s, i1m, i5m, range]() {
                    if (safeThis != nullptr) {
                        safeThis->loudnessAnalyserSettings.updateIndexDisplay(
                            i10s, i1m, i5m, range);
                    }
                });
            };
    }

    //==============================================================================
    // Component functions

    void resized() override {
        auto bounds = getLocalBounds();

        valueHistoryComp.setBounds(bounds);
        loudnessAnalyserSettings.setBounds(
            bounds.getProportion(juce::Rectangle(0.F, 0.f, 0.6f, 1.f)));
    }

    //==============================================================================
    // AudioAppComponent functions

    void pushNextSampleIntoFifo(float sample) noexcept {
        loudnessAnalyser.pushNextSampleIntoFifo(sample);
    }

    /** Update sample rate for frequency display and A-weighting calculations */
    void setSampleRate(double sampleRate) {
        loudnessAnalyser.setSampleRate(sampleRate);
        loudnessAnalyserSettings.setSampleRate(sampleRate);
    }

    //==============================================================================
    void paint(Graphics& g) override { g.fillAll(Colours::black); }

private:
    std::function<bool(float)> onLoudness;
    float lastLevelSent = -10.F; // set to strange value to start off with

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
                if (safeThis->loudnessAnalyser.inputRange.isEnabled()) {
                    safeThis->loudnessAnalyserSettings.updateObservedRangeDisplay(
                        safeThis->loudnessAnalyser.inputRange.getMin(),
                        safeThis->loudnessAnalyser.inputRange.getMax());
                }

                if (level != safeThis->lastLevelSent) {
                    if (safeThis->onLoudness && safeThis->onLoudness(level)) {
                        safeThis->lastLevelSent = level;
                    }
                }
            });
        },
        initialProcessingBandLow,
        initialProcessingBandHigh,
        initialSmoothing,
        initialDecayExponent,
        initialRangeInMin,
        initialRangeInMax,
        initialOutputMin,
        initialOutputMax,
        initialAdaptationRate};
    AnalyserSettings loudnessAnalyserSettings {loudnessAnalyser,
                                               initialProcessingBandLow,
                                               initialProcessingBandHigh,
                                               initialSmoothing,
                                               initialDecayExponent,
                                               initialRangeInMin,
                                               initialRangeInMax,
                                               initialOutputMin,
                                               initialOutputMax,
                                               initialAdaptationRate};

    ValueHistoryComponent valueHistoryComp;
};
}  // namespace Loudness
