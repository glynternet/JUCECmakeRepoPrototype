#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Loudness {

/**
 * @brief Read-only display for Loudness Index and Range values.
 *
 * Shows the three time-window indices (10s, 1m, 5m) and the dynamic range
 * as read-only labels. Updated via callback from the processing thread.
 *
 * Layout:
 *   Index    6.2 / 5.8 / 5.4
 *            10s   1m   5m
 *   Range    4.8
 */
class LoudnessIndexDisplay : public juce::Component {
public:
    LoudnessIndexDisplay() {
        using namespace juce;

        indexLabel.setText("Index", dontSendNotification);
        indexLabel.setJustificationType(Justification::centredRight);
        addAndMakeVisible(indexLabel);

        indexValue.setText("- / - / -", dontSendNotification);
        indexValue.setJustificationType(Justification::centredLeft);
        addAndMakeVisible(indexValue);

        indexTimeScales.setText("10s   1m   5m", dontSendNotification);
        indexTimeScales.setJustificationType(Justification::centredLeft);
        indexTimeScales.setFont(Font(10.0f));
        indexTimeScales.setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(indexTimeScales);

        rangeLabel.setText("Range", dontSendNotification);
        rangeLabel.setJustificationType(Justification::centredRight);
        addAndMakeVisible(rangeLabel);

        rangeValue.setText("-", dontSendNotification);
        rangeValue.setJustificationType(Justification::centredLeft);
        addAndMakeVisible(rangeValue);
    }

    /** Update display values (call from message thread) */
    void update(float i10s, float i1m, float i5m, float range) {
        auto indexText =
            juce::String::formatted("%.1f / %.1f / %.1f", i10s, i1m, i5m);
        indexValue.setText(indexText, juce::dontSendNotification);
        rangeValue.setText(juce::String(range, 1), juce::dontSendNotification);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        constexpr int labelWidth = 90;

        // Index row
        auto indexRow = bounds.removeFromTop(20);
        indexLabel.setBounds(indexRow.removeFromLeft(labelWidth));
        indexValue.setBounds(indexRow);

        // Time scale labels (smaller, beneath index values)
        auto timeScaleRow = bounds.removeFromTop(14);
        timeScaleRow.removeFromLeft(labelWidth);
        indexTimeScales.setBounds(timeScaleRow);

        // Range row
        auto rangeRow = bounds.removeFromTop(20);
        rangeLabel.setBounds(rangeRow.removeFromLeft(labelWidth));
        rangeValue.setBounds(rangeRow);
    }

    /** Preferred height for this component */
    static constexpr int preferredHeight = 54;

private:
    juce::Label indexLabel;
    juce::Label indexValue;
    juce::Label indexTimeScales;
    juce::Label rangeLabel;
    juce::Label rangeValue;
};

}  // namespace Loudness
