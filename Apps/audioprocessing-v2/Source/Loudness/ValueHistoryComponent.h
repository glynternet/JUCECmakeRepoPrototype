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
    ~ValueHistoryComponent() {}

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
            float x0 = width - x(i, historySize, width);
            float x1 = width - x(i + 1, historySize, width);
            float halfHeight = (float) height / 2;
            float x0yFromCentre = yFromCentre(
                avgLevelHistory[(historySize + latestValueIndex - i) % historySize],
                height);
            float level =
                avgLevelHistory[(historySize + latestValueIndex - i - 1) % historySize];
            const float x1yFromCentre = yFromCentre(level, height);
            const float proportionOfWidthCompleted = x1 / (float) width;

            g.setColour(juce::Colours::transparentBlack.interpolatedWith(
                brightViolet, jmap(level * proportionOfWidthCompleted, 0.15f, 1.f)));
            g.drawLine({x0, halfHeight + x0yFromCentre, x1, halfHeight + x1yFromCentre},
                       3);
            g.drawLine({x0, halfHeight - x0yFromCentre, x1, halfHeight - x1yFromCentre},
                       3);
        }
    }

    float x(int index, int datasetSize, int width)
    {
        return (float) jmap<int>(index, 0, datasetSize - 1, 0, width);
    }

    float yFromCentre(float value, int height)
    {
        return jmap(value, 0.0f, 1.0f, 0.f, (float) height / 2);
    }

    int historySize = 100;
    Slider historySizeSlider;
    Label historySizeLabel;

    float avgLevelHistory[maxHistorySize] = {};
    int latestValueIndex = 0;
};
} // namespace Loudness
