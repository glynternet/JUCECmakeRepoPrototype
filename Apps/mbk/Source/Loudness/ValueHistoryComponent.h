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
            rebuildPaths(bounds.getWidth(), bounds.getHeight());
            lastBounds = bounds;
        }

        const float width = (float) bounds.getWidth();

        // Draw all three paths - inactive zones collapse to centerline
        g.setGradientFill(
            ColourGradient(Colours::transparentBlack, 0, 0, brightViolet, width, 0, false));
        g.fillPath(purplePath);

        g.setGradientFill(
            ColourGradient(Colours::transparentBlack, 0, 0, warningYellow, width, 0, false));
        g.fillPath(yellowPath);

        g.setGradientFill(
            ColourGradient(Colours::transparentBlack, 0, 0, dangerRed, width, 0, false));
        g.fillPath(redPath);
    }

    void resized() override {
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
    }

    void setCompact(bool compact) {
        isCompact = compact;
        historySizeSlider.setVisible(!compact);
        historySizeLabel.setVisible(!compact);
    }

private:
    static constexpr float warningThreshold = 0.9f;
    static constexpr float dangerThreshold = 0.98f;
    static constexpr float maxDisplayLevel = 1.0f;

    bool isCompact = false;
    const Colour brightViolet {0xffba6bf5};
    const Colour warningYellow {0xffffd700};
    const Colour dangerRed {0xffff4444};

    static constexpr int zonePurple = 0;
    static constexpr int zoneYellow = 1;
    static constexpr int zoneRed = 2;

    int getZone(float level) const {
        if (level >= dangerThreshold) {
            return zoneRed;
        }
        if (level >= warningThreshold) {
            return zoneYellow;
        }
        return zonePurple;
    }

    void rebuildPaths(int width, int height) {
        purplePath.clear();
        yellowPath.clear();
        redPath.clear();

        const float halfHeight = (float) height / 2.0f;
        const float widthF = (float) width;

        // Calculate positions, heights, and zones for each sample
        std::vector<float> xPositions(historySize);
        std::vector<float> sampleHeights(historySize);
        std::vector<int> zones(historySize);
        std::vector<float> purpleHeights(historySize);
        std::vector<float> yellowHeights(historySize);
        std::vector<float> redHeights(historySize);

        for (int i = 0; i < historySize; ++i) {
            const int bufferIndex = (historySize + latestValueIndex - i) % historySize;
            const float level = std::min(levelHistory[bufferIndex], maxDisplayLevel);
            const float xPos = widthF - (widthF * (float) i / (float) (historySize - 1));
            const float sampleHeight = level * halfHeight;

            xPositions[i] = xPos;
            sampleHeights[i] = sampleHeight;
            zones[i] = getZone(level);

            // Assign height to exactly one zone based on level
            if (zones[i] == zoneRed) {
                purpleHeights[i] = 0.0f;
                yellowHeights[i] = 0.0f;
                redHeights[i] = sampleHeight;
            } else if (zones[i] == zoneYellow) {
                purpleHeights[i] = 0.0f;
                yellowHeights[i] = sampleHeight;
                redHeights[i] = 0.0f;
            } else {
                purpleHeights[i] = sampleHeight;
                yellowHeights[i] = 0.0f;
                redHeights[i] = 0.0f;
            }
        }

        buildSymmetricPath(purplePath, xPositions, purpleHeights, sampleHeights, zones, zonePurple, halfHeight);
        buildSymmetricPath(yellowPath, xPositions, yellowHeights, sampleHeights, zones, zoneYellow, halfHeight);
        buildSymmetricPath(redPath, xPositions, redHeights, sampleHeights, zones, zoneRed, halfHeight);

        pathNeedsRebuild = false;
    }

    // TODO: When transitioning between regions, interpolate so that each region only
    // extends to its threshold boundary. E.g., purple should stop at 0.9 and yellow/red
    // should start from their threshold, rather than one region extending to the other's
    // full height at the transition point.
    void buildSymmetricPath(Path& path,
                            const std::vector<float>& xPositions,
                            const std::vector<float>& heights,
                            const std::vector<float>& sampleHeights,
                            const std::vector<int>& zones,
                            int myZone,
                            float halfHeight) {
        const int size = (int) xPositions.size();
        if (size < 2) {
            return;
        }

        // Upper edge (right to left)
        path.startNewSubPath(xPositions[0], halfHeight - heights[0]);
        for (int i = 1; i < size; ++i) {
            const float prevHeight = heights[i - 1];
            const float currHeight = heights[i];
            const int prevZone = zones[i - 1];
            const int currZone = zones[i];

            if (prevHeight > 0.0f && currHeight == 0.0f) {
                // Ending (visible to invisible)
                if (myZone > currZone) {
                    // Going to lower zone: extend to curr x at curr's level, then drop
                    path.lineTo(xPositions[i], halfHeight - sampleHeights[i]);
                    path.lineTo(xPositions[i], halfHeight);
                } else {
                    // Going to higher zone: don't extend, drop at prev x
                    path.lineTo(xPositions[i - 1], halfHeight);
                    path.lineTo(xPositions[i], halfHeight);
                }
            } else if (prevHeight == 0.0f && currHeight > 0.0f) {
                // Starting (invisible to visible)
                if (myZone > prevZone) {
                    // Coming from lower zone: extend back to prev x at prev's level
                    path.lineTo(xPositions[i - 1], halfHeight - sampleHeights[i - 1]);
                    path.lineTo(xPositions[i], halfHeight - currHeight);
                } else {
                    // Coming from higher zone: don't extend back, jump at curr x
                    path.lineTo(xPositions[i], halfHeight);
                    path.lineTo(xPositions[i], halfHeight - currHeight);
                }
            } else {
                path.lineTo(xPositions[i], halfHeight - currHeight);
            }
        }

        // Lower edge (left to right, traversing indices from size-1 down to 0)
        // First point connects upper edge end to lower edge start
        path.lineTo(xPositions[size - 1], halfHeight + heights[size - 1]);

        for (int i = size - 2; i >= 0; --i) {
            const float prevHeight = heights[i + 1];  // Previous in traversal direction
            const float currHeight = heights[i];
            const int prevZone = zones[i + 1];
            const int currZone = zones[i];

            if (prevHeight > 0.0f && currHeight == 0.0f) {
                // Ending (visible to invisible)
                if (myZone > currZone) {
                    // Going to lower zone: extend to curr x at curr's level, then rise
                    path.lineTo(xPositions[i], halfHeight + sampleHeights[i]);
                    path.lineTo(xPositions[i], halfHeight);
                } else {
                    // Going to higher zone: don't extend, rise at prev x
                    path.lineTo(xPositions[i + 1], halfHeight);
                    path.lineTo(xPositions[i], halfHeight);
                }
            } else if (prevHeight == 0.0f && currHeight > 0.0f) {
                // Starting (invisible to visible)
                if (myZone > prevZone) {
                    // Coming from lower zone: extend back to prev x at prev's level
                    path.lineTo(xPositions[i + 1], halfHeight + sampleHeights[i + 1]);
                    path.lineTo(xPositions[i], halfHeight + currHeight);
                } else {
                    // Coming from higher zone: don't extend back, drop at curr x
                    path.lineTo(xPositions[i], halfHeight);
                    path.lineTo(xPositions[i], halfHeight + currHeight);
                }
            } else {
                path.lineTo(xPositions[i], halfHeight + currHeight);
            }
        }

        path.closeSubPath();
    }

    int historySize = 100;
    Slider historySizeSlider;
    Label historySizeLabel;

    float levelHistory[maxHistorySize] = {};
    int latestValueIndex = 0;

    Path purplePath;
    Path yellowPath;
    Path redPath;
    bool pathNeedsRebuild = true;
    Rectangle<int> lastBounds;
};
} // namespace Loudness
