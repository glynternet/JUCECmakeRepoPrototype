#pragma once

#include "JuceHeader.h"
#include "ValueHistoryComponent.h"
#include <array>

namespace Loudness {

/**
 * @brief Displays stacked ValueHistoryComponents for pipeline visualization.
 *
 * Shows the loudness value at each stage of the processing pipeline:
 * - Raw: Input to pipeline (unprocessed FFT loudness)
 * - Shaped: After ValueShaper range mapping
 * - Smooth: After Smoother EWMA filter
 * - Decay: After TailOff decay effect
 * - Clamp: Final clamp to [0.0, 1.0]
 *
 * Each stage is displayed in compact mode (no individual slider).
 * Use setHistorySize() to control all stages simultaneously.
 */
class PipelineViewComponent : public juce::Component {
public:
    static constexpr size_t stageCount = 5;

    PipelineViewComponent() {
        for (size_t i = 0; i < stageCount; ++i) {
            stages.at(i).setCompact(true);
            addAndMakeVisible(stages.at(i));

            labels.at(i).setText(stageNames.at(i), juce::dontSendNotification);
            labels.at(i).setJustificationType(juce::Justification::centredRight);
            labels.at(i).setColour(juce::Label::textColourId, juce::Colours::white);
            addAndMakeVisible(labels.at(i));
        }
    }

    /** Update all stage visualizations with new values */
    void updateValues(const std::array<float, stageCount>& values) {
        for (size_t i = 0; i < stageCount; ++i) {
            stages.at(i).addLevel(values.at(i));
        }
    }

    /** Set history size for all stages */
    void setHistorySize(int size) {
        for (auto& stage : stages) {
            stage.setHistorySize(size);
        }
    }

    void resized() override {
        auto bounds = getLocalBounds();
        const int labelWidth = 50;
        const int labelMargin = 4;
        const int stageGap = 2;
        const int totalGaps = static_cast<int>(stageCount) - 1;
        const int stageHeight =
            (bounds.getHeight() - (stageGap * totalGaps)) / static_cast<int>(stageCount);

        for (size_t i = 0; i < stageCount; ++i) {
            auto row = bounds.removeFromTop(stageHeight);
            labels.at(i).setBounds(row.removeFromLeft(labelWidth).reduced(0, labelMargin));
            stages.at(i).setBounds(row);

            if (i < stageCount - 1) {
                bounds.removeFromTop(stageGap);
            }
        }
    }

    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }

private:
    std::array<ValueHistoryComponent, stageCount> stages;
    std::array<juce::Label, stageCount> labels;

    static constexpr std::array<const char*, stageCount> stageNames = {
        "Raw", "Shaped", "Smooth", "Decay", "Clamp\n(final)"};
};

}  // namespace Loudness
