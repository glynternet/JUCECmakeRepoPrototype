#include "TempoSynthesizerComponent.h"
#include "Math.h"

namespace AudioApp
{
    static const int fadeIncrements = 8;
    int halfHeight;
    int segmentWidth;

    TempoSynthesizerComponent::TempoSynthesizerComponent(Logger& l) : logger(l) {
        up.onClick = [this](){
            if (nextMultipleIndex < MULTIPLE_COUNT-1) {
                setNextMultipleIndex(nextMultipleIndex + 1);
            }
        };
        juce::Path upShape;
        upShape.addTriangle(0.0, 0.0, 1.0, 1.0, 0.0, 2.0);
        up.setShape(upShape, true, false, false);
        up.setOutline(juce::Colours::transparentWhite, 3);
        addAndMakeVisible(up);
        down.onClick = [this](){
            if (nextMultipleIndex > 0) {
                setNextMultipleIndex(nextMultipleIndex - 1);
            }
        };
        juce::Path downShape;
        downShape.addTriangle(1.0, 0.0, 0.0, 1.0, 1.0, 2.0);
        down.setShape(downShape, true, false, false);
        down.setOutline(juce::Colours::transparentWhite, 3);
        addAndMakeVisible(down);
        for (int i = 0; i < MULTIPLE_COUNT; ++i) {
            juce::ShapeButton &button = multipleButtons[i];
            button.onClick = [this, i]() {
                setNextMultipleIndex(i);
            };
            juce::Path rect;
            rect.addRectangle(0.0, 0.0, 1.0, 1.0);
            button.setShape(rect, true, false, false);
            button.setOutline(juce::Colours::transparentWhite, 3);
            addAndMakeVisible(button);
        }
        setMultipleFromIndex(2);
        setNextMultipleIndex(2);
        juce::HighResolutionTimer::startTimer(1);
        juce::Timer::startTimerHz(60);
    }

    void TempoSynthesizerComponent::setMultipleFromIndex(int m) {
        multipleButtons[multipleIndex].setColours(juce::Colours::grey, juce::Colours::grey, juce::Colours::grey);
        multipleIndex = m;
        multiple = ipow(2, multipleIndex);
        segmentWidth = getWidth() / multiple;
        multipleButtons[multipleIndex].setColours(juce::Colours::white, juce::Colours::white, juce::Colours::white);
        dirty = true;
    }

    void TempoSynthesizerComponent::setNextMultipleIndex(int m) {
        multipleButtons[nextMultipleIndex].setOutline(juce::Colours::transparentWhite, 3);
        nextMultipleIndex = m;
        multipleButtons[nextMultipleIndex].setOutline(juce::Colours::white, 3);
        dirty = true;
    }

    void TempoSynthesizerComponent::paint(Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        g.setColour(colour);
        g.fillRect((int)((float)getWidth() * (float)currentBeat / (float)multiple), 0, segmentWidth, halfHeight);
    }

    void TempoSynthesizerComponent::resized() {
        halfHeight = getHeight() / 2;
        auto rect = getLocalBounds().withTrimmedTop(halfHeight);
        up.setBounds(rect.removeFromRight(30));
        down.setBounds(rect.removeFromLeft(30));
        for (int i = 0; i < MULTIPLE_COUNT; ++i) {
            multipleButtons[i].setBounds(rect.removeFromLeft(rect.getWidth() / (MULTIPLE_COUNT - i)));
        }
        segmentWidth = getWidth() / multiple;
    }

    void TempoSynthesizerComponent::timerCallback() {
        if (dirty.exchange(false)) {
            repaint();
        }
    }

    void TempoSynthesizerComponent::hiResTimerCallback() {
        while (!scheduledBeats.empty()) {
            auto soonest = scheduledBeats.front();
            // We call getMillisecondCounterHiRes() within this loop but are assuming that the loop
            // is rarely going to be ran for more than one iteration so it's probably
            // better most of the time to not call it before the loop and store it.
            // Although, this should definitely be benchmarked rather than worked on this
            // assumption.
            if (juce::Time::getMillisecondCounterHiRes() < soonest.millis) {
                break;
            }
            updateBeat(soonest.beat);
            scheduledBeats.pop_front();
        }
    }

    // beat is called on the beat detected with period being the time between each call
    void TempoSynthesizerComponent::beat(double period) {
        auto timeOfBeat = juce::Time::getMillisecondCounterHiRes();

        diffEwma = ewma(diffEwma, (double)period, 0.5);

        // We update the beat to 0 here because we currently only support synthesizing
        // extra beats rather than downsampling to less.
        updateBeat(0);

        if (multipleIndex != nextMultipleIndex) {
            setMultipleFromIndex(nextMultipleIndex);
        }

        for (uint8_t i = 0; i < multiple - 1; ++i) {
            uint8_t beat = i + 1;
            scheduledBeats.push_back(scheduledBeat{
                timeOfBeat + (diffEwma / (double) multiple * (double)beat),
                beat,
            });
        }

        flash(0.75f * (float)diffEwma);
        dirty = true;
    }

    void TempoSynthesizerComponent::flash(float duration) {
        Repeat::repeatFunc(duration/fadeIncrements, fadeIncrements, [this](int i){
            this->updateColour(juce::Colours::white.interpolatedWith(juce::Colours::grey, (float) i / float(fadeIncrements-1)));
        });
    }

    void TempoSynthesizerComponent::updateBeat(uint8_t beat) {
        currentBeat = beat;
        dirty = true;
    }

    void TempoSynthesizerComponent::updateColour(juce::Colour newColour) {
        colour = newColour;
        dirty = true;
    }
}