#pragma once

#include "JuceHeader.h"

namespace Loudness {
class ValueHistoryComponent : public Component {
public:
    ValueHistoryComponent() {
        addAndMakeVisible(historySizeSlider);
        historySizeSlider.setRange(2, ValueHistoryComponent::maxHistorySize);
        historySizeSlider.onValueChange = [this] {
            setHistorySize((int) historySizeSlider.getValue());
        };
        historySizeSlider.setValue(100);
        historySizeSlider.setTextBoxStyle(
            Slider::NoTextBox, false, 160, historySizeSlider.getTextBoxHeight());

        addAndMakeVisible(historySizeLabel);
        historySizeLabel.setText("History Size", dontSendNotification);
        historySizeLabel.attachToComponent(&historySizeSlider, true);
    }
    ~ValueHistoryComponent() = default;

    static const int maxHistorySize = 500;

    void addLevel(float level) {
        ++latestValueIndex;
        latestValueIndex %= historySize;
        levelHistory[latestValueIndex] = level;
        pathNeedsRebuild = true;
        repaint();
    }

    void paint(Graphics& g) override {
        g.fillAll(Colours::black);

        const auto bounds = getLocalBounds();
        if (bounds.isEmpty() || historySize < 2) {
            return;
        }

        if (pathNeedsRebuild || lastBounds != bounds) {
            rebuildPath(bounds.getWidth(), bounds.getHeight());
            lastBounds = bounds;
        }

        ColourGradient gradient(Colours::transparentBlack, 0, 0,
                                brightViolet, (float) bounds.getWidth(), 0, false);
        g.setGradientFill(gradient);
        g.strokePath(upperPath, PathStrokeType(3.0f));
        g.strokePath(lowerPath, PathStrokeType(3.0f));
    }

    void resized() override {
        pathNeedsRebuild = true;
        if (!isCompact) {
            const int sliderLeft = proportionOfWidth(0.69f);
            historySizeSlider.setBounds(sliderLeft, 10, getWidth() - sliderLeft - 10, 20);
        }
    }

    void setHistorySize(int size) {
        if (size > maxHistorySize || size < 2) {
            return;
        }
        historySize = size;
        pathNeedsRebuild = true;
    }

    void setCompact(bool compact) {
        isCompact = compact;
        historySizeSlider.setVisible(!compact);
        historySizeLabel.setVisible(!compact);
    }

private:
    bool isCompact = false;
    const Colour brightViolet {0xffba6bf5};

    void rebuildPath(int width, int height) {
        upperPath.clear();
        lowerPath.clear();

        const float halfHeight = (float) height / 2.0f;
        const float widthF = (float) width;

        for (int i = 0; i < historySize; ++i) {
            const int bufferIndex = (historySize + latestValueIndex - i) % historySize;
            const float level = levelHistory[bufferIndex];

            const float xPos = widthF - (widthF * (float) i / (float) (historySize - 1));
            const float yOffset = level * halfHeight;

            if (i == 0) {
                upperPath.startNewSubPath(xPos, halfHeight + yOffset);
                lowerPath.startNewSubPath(xPos, halfHeight - yOffset);
            } else {
                upperPath.lineTo(xPos, halfHeight + yOffset);
                lowerPath.lineTo(xPos, halfHeight - yOffset);
            }
        }

        pathNeedsRebuild = false;
    }

    int historySize = 100;
    Slider historySizeSlider;
    Label historySizeLabel;

    float levelHistory[maxHistorySize] = {};
    int latestValueIndex = 0;

    Path upperPath;
    Path lowerPath;
    bool pathNeedsRebuild = true;
    Rectangle<int> lastBounds;
};
} // namespace Loudness
