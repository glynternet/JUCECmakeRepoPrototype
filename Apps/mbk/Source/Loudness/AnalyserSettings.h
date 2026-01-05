#pragma once

#include "../Components/LabelledSlider.h"
#include "Analyser.h"

namespace Loudness {

class AnalyserSettings : public juce::Component {
public:
    explicit AnalyserSettings(Loudness::Analyser& loudnessAnalyser,
                              const double initialProcessingBandLow,
                              const double initialProcessingBandHigh,
                              const int movingAverageInitialWindow,
                              const float initialDecayExponent,
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
                0.0f,
                1.0f,
                0.1f,
                0.8f,
                0.5f,
                [&loudnessAnalyser](const double min, const double max) {
                    loudnessAnalyser.rangeAdapter.setObservedRange((float) min,
                                                                   (float) max);
                })
        ,

        outputRange("Output Range",
                    0.0f,
                    1.0f,
                    0.0f,
                    1.0f,
                    0.5f,
                    [&loudnessAnalyser](const double min, const double max) {
                        loudnessAnalyser.rangeAdapter.setTargetOutRange((float) min,
                                                                        (float) max);
                    })
        ,

        adaptationSpeed("Range Adapt Rate",
                        0.0001f,
                        0.05f,
                        0.005f,
                        0.01f,
                        [&loudnessAnalyser](const double value) {
                            loudnessAnalyser.rangeAdapter.setAdaptationRate(
                                (float) value);
                        })
        ,

        decayLength("Decay Length",
                    TailOff::minExponent,
                    TailOff::maxExponent,
                    initialDecayExponent,
                    jmap(0.90f, TailOff::minExponent, TailOff::maxExponent),
                    [&loudnessAnalyser](const double exponent) {
                        loudnessAnalyser.decayLength.setMaxDecayCoefficient(
                            (float) exponent);
                    })
        ,

        movingAverage("Window Size",
                      1.f,
                      7.f,
                      1.f,
                      movingAverageInitialWindow,
                      1.f,
                      [&loudnessAnalyser](const double value) {
                          loudnessAnalyser.movingAverage.setPeriod((int) value);
                      }) {
        addAndMakeVisible(rangeIn);
        addAndMakeVisible(outputRange);
        addAndMakeVisible(frequencyProcessingBand);
        addAndMakeVisible(decayLength);
        addAndMakeVisible(movingAverage);

        // Auto-range controls
        autoRangeEnabled.setToggleState(true, dontSendNotification);
        autoRangeEnabled.onClick = [&loudnessAnalyser, this]() {
            loudnessAnalyser.rangeAdapter.setEnabled(autoRangeEnabled.getToggleState());
        };
        addAndMakeVisible(autoRangeEnabled);

        rangeLocked.onClick = [&loudnessAnalyser, this]() {
            loudnessAnalyser.rangeAdapter.setLocked(rangeLocked.getToggleState());
        };
        addAndMakeVisible(rangeLocked);

        addAndMakeVisible(adaptationSpeed);

        // A-weighting toggle (on by default)
        aWeightingEnabled.setToggleState(true, dontSendNotification);
        aWeightingEnabled.onClick = [&loudnessAnalyser, this]() {
            loudnessAnalyser.setWeightingMode(aWeightingEnabled.getToggleState()
                                                  ? WeightingMode::AWeighted
                                                  : WeightingMode::Flat);
        };
        addAndMakeVisible(aWeightingEnabled);
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
    }

private:
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
        if (bounds.getHeight() > 330) {
            bounds = bounds.removeFromTop(330);
        }

        // Frequency band slider (has value label underneath)
        frequencyProcessingBand.setBounds(bounds.removeFromTop(40));

        // Observed input range slider (auto-updated from audio content)
        rangeIn.setBounds(bounds.removeFromTop(30));

        // Output range slider (user-controlled target output)
        outputRange.setBounds(bounds.removeFromTop(30));

        // Toggle buttons on their own row
        auto toggleRow = bounds.removeFromTop(25);
        toggleRow.removeFromLeft(90); // Align with sliders (skip label area)
        autoRangeEnabled.setBounds(toggleRow.removeFromLeft(60));
        rangeLocked.setBounds(toggleRow.removeFromLeft(60));
        aWeightingEnabled.setBounds(toggleRow.removeFromLeft(80));

        // Adaptation speed slider
        adaptationSpeed.setBounds(bounds.removeFromTop(30));

        auto remaining = bounds;
        decayLength.setBounds(remaining.removeFromTop(remaining.getHeight() / 2));
        movingAverage.setBounds(remaining);
    }

    double sampleRate;
    Components::LabelledSlider frequencyProcessingBand;
    Components::LabelledSlider rangeIn;
    Components::LabelledSlider outputRange;
    Components::LabelledSlider adaptationSpeed;
    juce::ToggleButton autoRangeEnabled {"Auto"};
    juce::ToggleButton rangeLocked {"Lock"};
    juce::ToggleButton aWeightingEnabled {"A-Weight"};
    Components::LabelledSlider decayLength;
    Components::LabelledSlider movingAverage;
};
} // namespace Loudness
