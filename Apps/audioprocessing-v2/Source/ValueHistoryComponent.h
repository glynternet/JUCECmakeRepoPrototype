#include "JuceHeader.h"

class ValueHistoryComponent : public Component {
   public:
    ValueHistoryComponent() {
        addAndMakeVisible(historySizeSlider);
        historySizeSlider.setRange(2, ValueHistoryComponent::maxHistorySize);  // [1]
        historySizeSlider.onValueChange = [this] { setHistorySize((int)historySizeSlider.getValue()); };
        historySizeSlider.setValue(100);
        historySizeSlider.setTextBoxStyle(Slider::NoTextBox, false, 160,
                                          historySizeSlider.getTextBoxHeight());

        addAndMakeVisible(historySizeLabel);
        historySizeLabel.setText("History Size", dontSendNotification);
        historySizeLabel.attachToComponent(&historySizeSlider, true);  // [4]
    }
    ~ValueHistoryComponent() {}

    static const int maxHistorySize = 500;

    void addLevel(float level) {
        ++latestValueIndex;
        latestValueIndex %= historySize;
        avgLevelHistory[latestValueIndex] = level;
        repaint();
    }

    void paint(Graphics& g) override {
        g.fillAll(Colours::blueviolet);

        auto width = getLocalBounds().getWidth();
        auto height = getLocalBounds().getHeight();

        drawHistoryLines(g, width, height);
        drawCurrentLevelIndicator(g, width, height);
    }

    void resized() override {
        const int sliderLeft = proportionOfWidth(0.69f);
        historySizeSlider.setBounds(sliderLeft, 10, getWidth() - sliderLeft - 10, 20);
    }

    void setHistorySize(int size) {
        if (size > maxHistorySize) {
            // TODO: Throw or something?
            return;
        }
        if (size < 2) {
            // TODO: Throw or something?
            return;
        }
        historySize = size;
    }

   private:
    void drawHistoryLines(Graphics& g, int width, int height) {
        // TODO(glynternet): no need calculate yFromCentre twice for each element
        for (int i = 1; i < historySize; ++i) {
            float x0 = x(i - 1, historySize, width);
            float x1 = x(i, historySize, width);
            float halfHeight = (float)height/2;
            float x0yFromCentre = yFromCentre(avgLevelHistory[i - 1], height);
            float x1yFromCentre = yFromCentre(avgLevelHistory[i], height);
            g.drawLine({x0,  halfHeight + x0yFromCentre, x1, halfHeight + x1yFromCentre});
            g.drawLine({x0,  halfHeight - x0yFromCentre, x1, halfHeight - x1yFromCentre});
        }
    }

    void drawCurrentLevelIndicator(Graphics& g, int width, int height) {
        float yDelta = yFromCentre(avgLevelHistory[latestValueIndex], height);
        float timeX = x(latestValueIndex, historySize, width);
        auto halfHeight = (float)height / 2.f;
        g.drawLine({timeX,  halfHeight-yDelta, timeX, halfHeight+yDelta});
    }

    float x(int index, int datasetSize, int width) {
        return (float)jmap<int>(index, 0, datasetSize - 1, 0, width);
    }

    float yFromCentre(float value, int height) { return jmap(value, 0.0f, 1.0f, 0.f, (float)height/2); }

    int historySize = 100;
    Slider historySizeSlider;
    Label historySizeLabel;

    float avgLevelHistory[maxHistorySize] = {};
    int latestValueIndex = 0;
};
