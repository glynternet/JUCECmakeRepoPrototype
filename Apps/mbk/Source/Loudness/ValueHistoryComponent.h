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

        // Calculate positions and heights for each sample
        std::vector<float> xPositions(historySize);
        std::vector<float> sampleHeights(historySize);
        std::vector<bool> purpleActive(historySize);
        std::vector<bool> yellowActive(historySize);
        std::vector<bool> redActive(historySize);

        for (int i = 0; i < historySize; ++i) {
            const int bufferIndex = (historySize + latestValueIndex - i) % historySize;
            const float level = std::min(levelHistory[bufferIndex], maxDisplayLevel);
            const float xPos = widthF - (widthF * (float) i / (float) (historySize - 1));

            xPositions[i] = xPos;
            sampleHeights[i] = level * halfHeight;

            // Mark exactly one zone as active based on level
            const int zone = getZone(level);
            purpleActive[i] = (zone == zonePurple);
            yellowActive[i] = (zone == zoneYellow);
            redActive[i] = (zone == zoneRed);
        }

        // Build paths for each zone. Each path covers all samples but only has non-zero height
        // where that zone is active. Higher priority zones (red > yellow > purple) visually
        // extend into lower priority zones at transitions, creating clean boundaries.
        buildSymmetricPath(purplePath, xPositions, purpleActive, sampleHeights, halfHeight);
        buildSymmetricPath(yellowPath, xPositions, yellowActive, sampleHeights, halfHeight);
        buildSymmetricPath(redPath, xPositions, redActive, sampleHeights, halfHeight);

        pathNeedsRebuild = false;
    }

    // Builds a symmetric path (mirrored above and below the centerline) for a single zone.
    //
    // The path traces the upper edge from right to left, then the lower edge from left to
    // right, creating a closed shape. Where active[i] is false, the path follows the centerline.
    //
    // Transition direction is determined by comparing adjacent sample heights:
    // - Height decreased: transitioning to a lower zone → extend to claim the boundary
    // - Height increased: transitioning to a higher zone → yield the boundary
    //
    // This makes higher priority zones visually "claim" boundaries by extending 1 sample
    // into lower priority territory.
    //
    // Parameters:
    // - path: Output path to build
    // - xPositions: X coordinate for each sample (index 0 = rightmost/newest)
    // - active: Whether this zone is active at each sample
    // - sampleHeights: Actual level height at each sample (regardless of zone)
    // - halfHeight: Half the component height (centerline y position)
    //
    // TODO: When transitioning between regions, interpolate so that each region only
    // extends to its threshold boundary. E.g., purple should stop at 0.9 and yellow/red
    // should start from their threshold, rather than one region extending to the other's
    // full height at the transition point.
    void buildSymmetricPath(Path& path,
                            const std::vector<float>& xPositions,
                            const std::vector<bool>& active,
                            const std::vector<float>& sampleHeights,
                            float halfHeight) {
        const int size = (int) xPositions.size();
        if (size < 2) {
            return;
        }

        // Upper edge (right to left)
        path.startNewSubPath(xPositions[0], halfHeight - (active[0] ? sampleHeights[0] : 0.0f));
        for (int i = 1; i < size; ++i) {
            const bool prevActive = active[i - 1];
            const bool currActive = active[i];

            if (prevActive && !currActive) {
                // Ending (visible to invisible)
                if (sampleHeights[i] < sampleHeights[i - 1]) {
                    // Height decreased: going to lower zone, extend then drop
                    path.lineTo(xPositions[i], halfHeight - sampleHeights[i]);
                    path.lineTo(xPositions[i], halfHeight);
                } else {
                    // Height increased: going to higher zone, drop at prev x
                    path.lineTo(xPositions[i - 1], halfHeight);
                    path.lineTo(xPositions[i], halfHeight);
                }
            } else if (!prevActive && currActive) {
                // Starting (invisible to visible)
                if (sampleHeights[i] > sampleHeights[i - 1]) {
                    // Height increased: coming from lower zone, extend back
                    path.lineTo(xPositions[i - 1], halfHeight - sampleHeights[i - 1]);
                    path.lineTo(xPositions[i], halfHeight - sampleHeights[i]);
                } else {
                    // Height decreased: coming from higher zone, jump at curr x
                    path.lineTo(xPositions[i], halfHeight);
                    path.lineTo(xPositions[i], halfHeight - sampleHeights[i]);
                }
            } else {
                path.lineTo(xPositions[i], halfHeight - (currActive ? sampleHeights[i] : 0.0f));
            }
        }

        // Lower edge (left to right, traversing indices from size-1 down to 0)
        // First point connects upper edge end to lower edge start
        const float endHeight = active[size - 1] ? sampleHeights[size - 1] : 0.0f;
        path.lineTo(xPositions[size - 1], halfHeight + endHeight);

        for (int i = size - 2; i >= 0; --i) {
            const bool prevActive = active[i + 1];  // Previous in traversal direction
            const bool currActive = active[i];

            if (prevActive && !currActive) {
                // Ending (visible to invisible)
                if (sampleHeights[i] < sampleHeights[i + 1]) {
                    // Height decreased: going to lower zone, extend then rise
                    path.lineTo(xPositions[i], halfHeight + sampleHeights[i]);
                    path.lineTo(xPositions[i], halfHeight);
                } else {
                    // Height increased: going to higher zone, rise at prev x
                    path.lineTo(xPositions[i + 1], halfHeight);
                    path.lineTo(xPositions[i], halfHeight);
                }
            } else if (!prevActive && currActive) {
                // Starting (invisible to visible)
                if (sampleHeights[i] > sampleHeights[i + 1]) {
                    // Height increased: coming from lower zone, extend back
                    path.lineTo(xPositions[i + 1], halfHeight + sampleHeights[i + 1]);
                    path.lineTo(xPositions[i], halfHeight + sampleHeights[i]);
                } else {
                    // Height decreased: coming from higher zone, drop at curr x
                    path.lineTo(xPositions[i], halfHeight);
                    path.lineTo(xPositions[i], halfHeight + sampleHeights[i]);
                }
            } else {
                path.lineTo(xPositions[i], halfHeight + (currActive ? sampleHeights[i] : 0.0f));
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
