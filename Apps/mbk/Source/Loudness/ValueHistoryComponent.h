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
        // TODO(glynternet): no need calculate yFromCentre twice for each element
        for (int i = 0; i < historySize - 1; ++i)
        {
            const auto l0index = i;
            const float l0X = width - x(l0index, historySize, width);
            const float l0 = avgLevelHistory[(historySize + latestValueIndex - l0index) % historySize];
            const auto l0YFromCentre = (float) proportionOfHeight(0.5f*l0);

            const auto l1index = i + 1;
            const float l1X = width - x(l1index, historySize, width);
            const float l1 = avgLevelHistory[(historySize + latestValueIndex - l1index) % historySize];
            const auto l1YFromCentre = (float) proportionOfHeight(0.5f*l1);

            const float proportionOfWidthCompleted = l1X / (float) width;
            g.setColour(juce::Colours::transparentBlack.interpolatedWith(
                brightViolet, jmap(l1 * proportionOfWidthCompleted, 0.15f, 1.f)));

            float halfHeight = (float) height / 2;
            g.drawLine({l0X, halfHeight + l0YFromCentre, l1X, halfHeight + l1YFromCentre},
                       3);
            g.drawLine({l0X, halfHeight - l0YFromCentre, l1X, halfHeight - l1YFromCentre},
                       3);
        }
    }

    float x(int index, int datasetSize, int width)
    {
        return (float) jmap<int>(index, 0, datasetSize - 1, 0, width);
    }

    int historySize = 100;
    Slider historySizeSlider;
    Label historySizeLabel;

    float avgLevelHistory[maxHistorySize] = {};
    int latestValueIndex = 0;
};
} // namespace Loudness
