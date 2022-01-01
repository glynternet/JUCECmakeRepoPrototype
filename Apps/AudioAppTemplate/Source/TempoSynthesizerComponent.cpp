#include "TempoSynthesizerComponent.h"

namespace AudioApp
{
    static const int fadeIncrements = 8;

    TempoSynthesizerComponent::TempoSynthesizerComponent() {
        label.setColour (juce::Label::textColourId, colour);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        startTimerHz(30);
    }

    void TempoSynthesizerComponent::paint(Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        label.setColour (juce::Label::textColourId, colour);
        label.setText(content, juce::dontSendNotification);
    }

    void TempoSynthesizerComponent::resized() {
        label.setBounds(getLocalBounds());
    }

    void TempoSynthesizerComponent::timerCallback() {
        if (dirty.exchange(false)) {
            repaint();
        }
    }

    void TempoSynthesizerComponent::beat(double period) {
        // this might actually be better as a function that says "repeat X time in the next Y milliseconds@
        colour = juce::Colours::white;
        for (int i = 0; i < fadeIncrements; ++i) {
            const double proportion = (float) i / float(fadeIncrements);
            // beatDueInCurrentFrame only happens every other beat and we want to fade over 2 beats, so we do
            // 1500 * seconds per beat to take 75% of the time between flashes to fade.
            // * 1000 to convert seconds to milliseconds
            // * 0.75 to convert to 75% of the time between "beats"
            // * 2 because the beat detection happens every other beat (there may be something in the research paper that mentions why this is)
            juce::Timer::callAfterDelay((int)((double)period * 0.75 * proportion), [this, proportion]{
                colour = juce::Colours::white.interpolatedWith(juce::Colours::grey, (float)proportion);
                dirty = true;
            });
        }

        ++beats;
        currentBeat = ++currentBeat%4;
        const int multiplier = 16;
        repeatFunc((int)(period/(double)multiplier), multiplier-1, [this]{
            currentBeat = ++currentBeat%4;
            this->updateLabel(currentBeat);
        });
    }

    void TempoSynthesizerComponent::updateLabel(uint8_t beat) {
        content = "● " + std::to_string(beat);
        dirty = true;
    }

    void TempoSynthesizerComponent::repeatFunc(int interval, int count, const std::function<void()>& call) {
        // TODO: use a single timer here instead
        for (int i = 0; i < count; ++i) {
            juce::Timer::callAfterDelay((1+i)*interval, call);
        }
    }
}