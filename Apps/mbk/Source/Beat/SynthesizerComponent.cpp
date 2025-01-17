#include "SynthesizerComponent.h"
#include "../Math.h"

namespace Beat
{
    double durationPerSynthesizedBeat = 500;

    SynthesizerComponent::SynthesizerComponent(logger::Logger& l) : logger(l) {
        up.onClick = [this](){
            if (nextMultipleIndex < (TOTAL_MULTIPLE_COUNT - 1)) {
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
        for (int i = 0; i < TOTAL_MULTIPLE_COUNT; ++i) {
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
        setMultipleFromIndex(4);
        setNextMultipleIndex(4);
        juce::HighResolutionTimer::startTimer(1);
        juce::Timer::startTimerHz(60);
    }

    void SynthesizerComponent::setMultipleFromIndex(int m) {
        multipleButtons[multipleIndex].setColours(juce::Colours::grey, juce::Colours::grey, juce::Colours::grey);
        multipleIndex = m;
        auto exponent = multipleIndex - NEGATIVE_MULTIPLE_COUNT;
        if (exponent < 0) {
            exponent = -exponent;
        }
        multiple = ipow(2, exponent);
        multipleButtons[multipleIndex].setColours(juce::Colours::white, juce::Colours::white, juce::Colours::white);
        dirty = true;
    }

    void SynthesizerComponent::setNextMultipleIndex(int m) {
        multipleButtons[nextMultipleIndex].setOutline(juce::Colours::transparentWhite, 3);
        nextMultipleIndex = m;
        multipleButtons[nextMultipleIndex].setOutline(juce::Colours::white, 3);
        dirty = true;
    }

    void SynthesizerComponent::paint(juce::Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void SynthesizerComponent::resized() {
        auto rect = getLocalBounds();
        up.setBounds(rect.removeFromRight(30));
        down.setBounds(rect.removeFromLeft(30));
        for (int i = 0; i < TOTAL_MULTIPLE_COUNT; ++i) {
            multipleButtons[i].setBounds(rect.removeFromLeft(rect.getWidth() / (TOTAL_MULTIPLE_COUNT - i)));
        }
    }

    void SynthesizerComponent::timerCallback() {
        if (dirty.exchange(false)) {
            repaint();
        }
    }

    void SynthesizerComponent::hiResTimerCallback() {
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
            synthesizedBeat(durationPerSynthesizedBeat);
            scheduledBeats.pop_front();
        }
    }

    // beat is called on the beat detected with period being the time between each call
    void SynthesizerComponent::beat(double period) {
        auto timeOfBeat = juce::Time::getMillisecondCounterHiRes();
        inputBeatCount++;
        diffEwma = ewma(diffEwma, (double)period, 0.5);

        if (multipleIndex != nextMultipleIndex) {
            setMultipleFromIndex(nextMultipleIndex);
        }

        // map multipleIndex to a value where positive is upsampling and negative is downsampling a beat.
        auto relativeMultiplierIndex = multipleIndex - NEGATIVE_MULTIPLE_COUNT;

        // upsampling
        if (relativeMultiplierIndex > 0) {
            durationPerSynthesizedBeat = diffEwma / (double) multiple;
            synthesizedBeat(durationPerSynthesizedBeat);
            for (uint8_t i = 0; i < multiple - 1; ++i) {
                scheduledBeats.push_back(scheduledBeat{
                        timeOfBeat + durationPerSynthesizedBeat * (double)(i + 1),
                });
            }

        // synthesized is the same as input
        } else if (relativeMultiplierIndex == 0) {
            durationPerSynthesizedBeat = diffEwma;
            synthesizedBeat(durationPerSynthesizedBeat);

        // downsampling
        } else {
            durationPerSynthesizedBeat = diffEwma * (double) multiple;
            if (inputBeatCount % multiple == 0) {
                synthesizedBeat(durationPerSynthesizedBeat);
            }
        }

        dirty = true;
    }

    void SynthesizerComponent::synthesizedBeat(double duration) {
        if (onSynthesizedBeat != nullptr) {
            onSynthesizedBeat(duration);
        }
    }
}