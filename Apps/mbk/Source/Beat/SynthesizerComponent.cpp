#include "SynthesizerComponent.h"

namespace Beat {

SynthesizerComponent::SynthesizerComponent(Synthesizer& synth)
    : synthesizer(synth) {
    up.onClick = [this]() {
        int next = synthesizer.getNextMultipleIndex();
        if (next < Synthesizer::totalMultipleCount - 1) {
            synthesizer.setNextMultipleIndex(next + 1);
            dirty = true;
        }
    };
    juce::Path upShape;
    upShape.addTriangle(0.0, 0.0, 1.0, 1.0, 0.0, 2.0);
    up.setShape(upShape, true, false, false);
    up.setOutline(juce::Colours::transparentWhite, 3);
    addAndMakeVisible(up);

    down.onClick = [this]() {
        int next = synthesizer.getNextMultipleIndex();
        if (next > 0) {
            synthesizer.setNextMultipleIndex(next - 1);
            dirty = true;
        }
    };
    juce::Path downShape;
    downShape.addTriangle(1.0, 0.0, 0.0, 1.0, 1.0, 2.0);
    down.setShape(downShape, true, false, false);
    down.setOutline(juce::Colours::transparentWhite, 3);
    addAndMakeVisible(down);

    for (size_t i = 0; i < Synthesizer::totalMultipleCount; ++i) {
        juce::ShapeButton& button = multipleButtons.at(i);
        button.onClick = [this, i]() {
            synthesizer.setNextMultipleIndex(static_cast<int>(i));
            dirty = true;
        };
        juce::Path rect;
        rect.addRectangle(0.0, 0.0, 1.0, 1.0);
        button.setShape(rect, true, false, false);
        button.setOutline(juce::Colours::transparentWhite, 3);
        addAndMakeVisible(button);
    }

    startTimerHz(60);
}

void SynthesizerComponent::updateButtonStates() {
    int currentIndex = synthesizer.getMultipleIndex();
    int nextIndex = synthesizer.getNextMultipleIndex();

    if (currentIndex != lastMultipleIndex) {
        if (lastMultipleIndex >= 0 && lastMultipleIndex < Synthesizer::totalMultipleCount) {
            multipleButtons.at(static_cast<size_t>(lastMultipleIndex))
                .setColours(juce::Colours::grey, juce::Colours::grey, juce::Colours::grey);
        }
        multipleButtons.at(static_cast<size_t>(currentIndex))
            .setColours(juce::Colours::white, juce::Colours::white, juce::Colours::white);
        lastMultipleIndex = currentIndex;
    }

    if (nextIndex != lastNextMultipleIndex) {
        if (lastNextMultipleIndex >= 0 &&
            lastNextMultipleIndex < Synthesizer::totalMultipleCount) {
            multipleButtons.at(static_cast<size_t>(lastNextMultipleIndex))
                .setOutline(juce::Colours::transparentWhite, 3);
        }
        multipleButtons.at(static_cast<size_t>(nextIndex)).setOutline(juce::Colours::white, 3);
        lastNextMultipleIndex = nextIndex;
    }
}

void SynthesizerComponent::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SynthesizerComponent::resized() {
    auto rect = getLocalBounds();
    up.setBounds(rect.removeFromRight(30));
    down.setBounds(rect.removeFromLeft(30));
    for (size_t i = 0; i < Synthesizer::totalMultipleCount; ++i) {
        int remaining = Synthesizer::totalMultipleCount - static_cast<int>(i);
        multipleButtons.at(i).setBounds(rect.removeFromLeft(rect.getWidth() / remaining));
    }
}

void SynthesizerComponent::timerCallback() {
    updateButtonStates();
    if (dirty.exchange(false)) {
        repaint();
    }
}

} // namespace Beat
