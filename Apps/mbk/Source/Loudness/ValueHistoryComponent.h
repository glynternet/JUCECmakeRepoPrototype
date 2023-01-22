#include "JuceHeader.h"

namespace Loudness
{
class ValueHistoryComponent : public Component
{
public:
    ValueHistoryComponent()
    {
        addAndMakeVisible(historySizeSlider);
        historySizeSlider.setRange(2, ValueHistoryComponent::maxHistorySize); // [1]
        historySizeSlider.onValueChange = [this]
        { setHistorySize((int) historySizeSlider.getValue()); };
        historySizeSlider.setValue(100);
        historySizeSlider.setTextBoxStyle(
            Slider::NoTextBox, false, 160, historySizeSlider.getTextBoxHeight());

        addAndMakeVisible(historySizeLabel);
        historySizeLabel.setText("History Size", dontSendNotification);
        historySizeLabel.attachToComponent(&historySizeSlider, true); // [4]
    }
    ~ValueHistoryComponent() = default;

    static const int maxHistorySize = 500;

    void addLevel(float level)
    {
        ++latestValueIndex;
        latestValueIndex %= historySize;
        avgLevelHistory[latestValueIndex] = level;
        repaint();
    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colours::black);

        auto width = getLocalBounds().getWidth();
        auto height = getLocalBounds().getHeight();

        drawHistoryLines(g, width, height);
    }

    void resized() override
    {
        const int sliderLeft = proportionOfWidth(0.69f);
        historySizeSlider.setBounds(sliderLeft, 10, getWidth() - sliderLeft - 10, 20);
    }

    void setHistorySize(int size)
    {
        if (size > maxHistorySize)
        {
            // TODO: Throw or something?
            return;
        }
        if (size < 2)
        {
            // TODO: Throw or something?
            return;
        }
        historySize = size;
    }

private:
    const Colour brightViolet {0xffba6bf5};

    void drawHistoryLines(Graphics& g, int width, int height)
    {
        const float halfHeight = (float) height / 2.f;

        float l0X = (float) width - x(0, historySize);
        const float l0 = avgLevelHistory[(historySize + latestValueIndex - 0) % historySize];
        const auto l0YFromCentre = (float) proportionOfHeight(0.5f*l0);
        auto l0YUpper = halfHeight+l0YFromCentre;
        auto l0YLower = halfHeight-l0YFromCentre;

        for (int i = 1; i < historySize; ++i) {
            const float l1X = (float) width - x(i, historySize);
            const float l1 = avgLevelHistory[(historySize + latestValueIndex - i) % historySize];
            const auto l1YFromCentre = (float) proportionOfHeight(0.5f*l1);

            const float proportionOfWidthCompleted = l1X / (float) width;
            g.setColour(juce::Colours::transparentBlack.interpolatedWith(
                brightViolet, jmap(l1 * proportionOfWidthCompleted, 0.15f, 1.f)));

            float l1YUpper = halfHeight + l1YFromCentre;
            g.drawLine({l0X, l0YUpper, l1X, l1YUpper}, 3);

            float l1YLower = halfHeight - l1YFromCentre;
            g.drawLine({l0X, l0YLower, l1X, l1YLower}, 3);

            l0X = l1X;
            l0YUpper = l1YUpper;
            l0YLower = l1YLower;
        }
    }

    float x(int index, int datasetSize) {
        return (float)proportionOfWidth((float)index/(float)(datasetSize-1));
    }

    int historySize = 100;
    Slider historySizeSlider;
    Label historySizeLabel;

    float avgLevelHistory[maxHistorySize] = {};
    int latestValueIndex = 0;
};
} // namespace Loudness
