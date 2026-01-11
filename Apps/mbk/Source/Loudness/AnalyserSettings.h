#pragma once

#include "../Components/LabelledSlider.h"
#include "Analyser.h"
#include "LoudnessIndexDisplay.h"

namespace Loudness {

class AnalyserSettings : public juce::Component {
public:
    explicit AnalyserSettings(Loudness::Analyser& loudnessAnalyser,
                              const double initialProcessingBandLow,
                              const double initialProcessingBandHigh,
                              const float initialSmoothing,
                              const float initialDecayExponent,
                              juce::Range<float> initialRangeIn,
                              juce::Range<float> initialOutput,
                              const float initialAdaptationRate,
                              const bool initialAutoRangeEnabled,
                              const bool initialAWeightingEnabled,
                              const bool initialPerceptualMode,
                              double initialSampleRate = 48000.0)
        : sampleRate(initialSampleRate)
        // Bin range is [0, maxBins-1]. Proportions are converted to bins by truncation
        // (e.g., 0.02 * 128 = 2.56 → bin 2). The inverse division by maxBins means
        // bin 127 maps to 0.992 (not 1.0), which is correct since bin 127's upper
        // edge is Nyquist, and we want proportions to represent bin indices, not edges.
        , frequencyProcessingBand(
              "Frequency Band",
              0,
              Analyser::maxBins - 1,
              static_cast<int>(initialProcessingBandLow * Analyser::maxBins),
              static_cast<int>(initialProcessingBandHigh * Analyser::maxBins),
              [&loudnessAnalyser](int low, int high) {
                  loudnessAnalyser.processingBandLow =
                      static_cast<double>(low) / Analyser::maxBins;
                  loudnessAnalyser.processingBandHigh =
                      static_cast<double>(high) / Analyser::maxBins;
              },
              [this](int low, int high) { return formatFrequencyRange(low, high); })
        ,

        // Observed input range (learned from audio content, editable)
        rangeIn("Range In",
                0.0F,
                1.0F,
                initialRangeIn.getStart(),
                initialRangeIn.getEnd(),
                1.0F,
                [&loudnessAnalyser](const double min, const double max) {
                    loudnessAnalyser.inputRange.setBounds({(float) min, (float) max});
                },
                [](double min, double max) {
                    return String(min, 3) + " - " + String(max, 3);
                })
        ,

        outputRange("Output Range",
                    0.0F,
                    1.0F,
                    initialOutput.getStart(),
                    initialOutput.getEnd(),
                    1.0F,
                    [&loudnessAnalyser](const double min, const double max) {
                        loudnessAnalyser.setTargetOutRange({(float) min, (float) max});
                    })
        ,

        adaptationSpeed("Auto Adapt Rate",
                        0.0001F,
                        0.05F,
                        initialAdaptationRate,
                        0.01F,
                        [&loudnessAnalyser](const double value) {
                            loudnessAnalyser.inputRange.setAdaptationRate((float) value);
                        })
        ,

        decayLength("Decay Length",
                    TailOff::minExponent,
                    TailOff::maxExponent,
                    initialDecayExponent,
                    jmap(0.90F, TailOff::minExponent, TailOff::maxExponent),
                    [&loudnessAnalyser](const double exponent) {
                        loudnessAnalyser.decayLength.setMaxDecayCoefficient(
                            (float) exponent);
                    })
        ,

        smoothing("Smoothing",
                  0.F,
                  1.F,
                  initialSmoothing,
                  0.5F,
                  [&loudnessAnalyser](const double value) {
                      loudnessAnalyser.smoother.setSmoothing((float) value);
                  }) {
        addAndMakeVisible(rangeIn);
        addAndMakeVisible(outputRange);
        addAndMakeVisible(frequencyProcessingBand);
        addAndMakeVisible(decayLength);
        addAndMakeVisible(smoothing);

        // Auto-range controls
        autoRangeEnabled.setToggleState(initialAutoRangeEnabled, dontSendNotification);
        autoRangeEnabled.onClick = [&loudnessAnalyser, this]() {
            loudnessAnalyser.inputRange.setEnabled(autoRangeEnabled.getToggleState());
            updateAutoRelatedVisibility();
        };
        addAndMakeVisible(autoRangeEnabled);

        rangeLocked.onClick = [&loudnessAnalyser, this]() {
            loudnessAnalyser.inputRange.setLocked(rangeLocked.getToggleState());
        };
        rangeLocked.setVisible(initialAutoRangeEnabled);
        addChildComponent(rangeLocked);

        addAndMakeVisible(adaptationSpeed);

        // A-weighting toggle (frequency-corrected perception)
        aWeightingEnabled.setToggleState(initialAWeightingEnabled, dontSendNotification);
        aWeightingEnabled.onClick = [&loudnessAnalyser, this]() {
            loudnessAnalyser.setWeightingMode(aWeightingEnabled.getToggleState()
                                                  ? WeightingMode::AWeighted
                                                  : WeightingMode::Flat);
        };
        addAndMakeVisible(aWeightingEnabled);

        // Perceptual loudness toggle (Stevens' Power Law)
        // When enabled, uses power-domain calculation with Stevens exponent (0.3)
        // for true perceptual linearity where 0.5 feels half as loud as 1.0.
        // When disabled, uses legacy linear amplitude mapping.
        perceptualModeEnabled.setToggleState(initialPerceptualMode, dontSendNotification);
        perceptualModeEnabled.onClick = [&loudnessAnalyser, this]() {
            loudnessAnalyser.setMappingMode(perceptualModeEnabled.getToggleState()
                                                ? MappingMode::Perceptual
                                                : MappingMode::Linear);
        };
        addAndMakeVisible(perceptualModeEnabled);

        addAndMakeVisible(indexDisplay);
    }

    /** Update sample rate and refresh frequency display */
    void setSampleRate(double newSampleRate) {
        sampleRate = newSampleRate;
        frequencyProcessingBand.updateValueLabel();
    }

    /** Update observed input range display (for visual feedback) */
    void updateObservedRangeDisplay(float min, float max) {
        rangeIn.setMinValue(min);
        rangeIn.setMaxValue(max);
        rangeIn.updateValueLabel();
    }

    /** Update loudness index display (called from message thread via callback) */
    void updateIndexDisplay(float i10s, float i1m, float i5m, float range) {
        indexDisplay.update(i10s, i1m, i5m, range);
    }

private:
    void updateAutoRelatedVisibility() {
        const auto autoEnabled = autoRangeEnabled.getToggleState();
        adaptationSpeed.setVisible(autoEnabled);
        rangeLocked.setVisible(autoEnabled);
        resized();
    }

    /** Format a frequency value as Hz or kHz for display */
    static String formatHz(double hz) {
        if (hz >= 1000.0)
            return String(hz / 1000.0, 1) + " kHz";
        return String(static_cast<int>(hz)) + " Hz";
    }

    /** Convert bin index to its lower edge frequency in Hz */
    double binLowerEdgeHz(int bin) const {
        return static_cast<double>(bin) * sampleRate / Analyser::fftSize;
    }

    /** Convert bin index to its upper edge frequency in Hz */
    double binUpperEdgeHz(int bin) const {
        return static_cast<double>(bin + 1) * sampleRate / Analyser::fftSize;
    }

    /** Format frequency range for display (e.g., "375 Hz - 3.2 kHz") */
    String formatFrequencyRange(int binLow, int binHigh) const {
        return formatHz(binLowerEdgeHz(binLow)) + " - "
               + formatHz(binUpperEdgeHz(binHigh));
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(10);

        // UI ordered to match processing pipeline:
        // 1. Band Selection → 2. A-Weighting → 3. Loudness Calc →
        // 4. Adaptive Range → 5. Value Shaping → 6. Smoothing → 7. Decay

        // 1. Frequency band slider (has value label underneath)
        frequencyProcessingBand.setBounds(bounds.removeFromTop(40));

        // 2-3. Loudness calculation toggles row (A-weighting, then perceptual mode)
        auto loudnessRow = bounds.removeFromTop(25);
        loudnessRow.removeFromLeft(90); // Align with sliders (skip label area)
        aWeightingEnabled.setBounds(loudnessRow.removeFromLeft(70));
        perceptualModeEnabled.setBounds(loudnessRow.removeFromLeft(80));

        // 4. Observed input range slider (auto-updated from audio content, has value label)
        rangeIn.setBounds(bounds.removeFromTop(40));

        // 4. Auto range toggles row
        auto autoRangeRow = bounds.removeFromTop(25);
        autoRangeRow.removeFromLeft(90); // Align with sliders (skip label area)
        autoRangeEnabled.setBounds(autoRangeRow.removeFromLeft(50));
        if (rangeLocked.isVisible()) {
            rangeLocked.setBounds(autoRangeRow.removeFromLeft(50));
        }

        // 4. Adaptation speed slider (only shown when Auto is enabled)
        if (adaptationSpeed.isVisible()) {
            adaptationSpeed.setBounds(bounds.removeFromTop(30));
        }

        // 5. Output range slider (user-controlled target output)
        outputRange.setBounds(bounds.removeFromTop(30));

        // 6. Smoothing
        smoothing.setBounds(bounds.removeFromTop(30));

        // 7. Decay
        decayLength.setBounds(bounds.removeFromTop(30));

        // Loudness Index display (statistics output, not a pipeline stage)
        indexDisplay.setBounds(
            bounds.removeFromTop(LoudnessIndexDisplay::preferredHeight));
    }

    double sampleRate;
    Components::LabelledSlider frequencyProcessingBand;
    Components::LabelledSlider rangeIn;
    Components::LabelledSlider outputRange;
    Components::LabelledSlider adaptationSpeed;
    juce::ToggleButton autoRangeEnabled {"Auto"};
    juce::ToggleButton rangeLocked {"Lock"};
    juce::ToggleButton aWeightingEnabled {"A-Weight"};
    juce::ToggleButton perceptualModeEnabled {"Percept"};
    Components::LabelledSlider decayLength;
    Components::LabelledSlider smoothing;
    LoudnessIndexDisplay indexDisplay;
};
}  // namespace Loudness
