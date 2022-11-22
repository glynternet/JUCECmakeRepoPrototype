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

        if (drawAverageLevelHistory) {
            drawHistoryLine(g, width, height);
        }
        drawCurrentLevelIndicator(g, width, height);
        drawCurrentTimeIndicator(g, width, height);
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
    void drawHistoryLine(Graphics& g, int width, int height) {
        for (int i = 1; i < historySize; ++i) {
            float x0 = x(i - 1, historySize, width);
            float x1 = x(i, historySize, width);

            drawLineSegment(g, x0, x1, i, height, avgLevelHistory);
        }
    }

    void drawCurrentLevelIndicator(Graphics& g, int width, int height) {
        float avgY = y(avgLevelHistory[latestValueIndex], height);
        float timeX = x(latestValueIndex, historySize, width);
        const float indicatorSize = 10.f;
        float indicatorX = timeX - indicatorSize / 2.f;
        float indicatorY = avgY - indicatorSize / 2.f;
        g.fillEllipse(indicatorX, indicatorY, indicatorSize, indicatorSize);
    }

    void drawCurrentTimeIndicator(Graphics& g, int width, int height) {
        auto timeX = x(latestValueIndex, historySize, width);
        g.fillRect((int)timeX, (int)0, 2, height);
    }

    void drawLineSegment(Graphics& g, float x0, float x1, int index, int height, float levels[]) {
        g.drawLine({x0, y(levels[index - 1], height), x1, y(levels[index], height)});
    }

    float x(int index, int datasetSize, int width) {
        return (float)jmap<int>(index, 0, datasetSize - 1, 0, width);
    }

    float y(float value, int height) { return jmap(value, 0.0f, 1.0f, (float)height, 0.0f); }

    int historySize = 100;
    Slider historySizeSlider;
    Label historySizeLabel;

    bool drawAverageLevelHistory = true;
    float avgLevelHistory[maxHistorySize] = {};
    int latestValueIndex = 0;
};
