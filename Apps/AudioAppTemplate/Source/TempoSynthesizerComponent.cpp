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

    void TempoSynthesizerComponent::beat(long long period) {
        diffEwma = ewma(diffEwma, (double)period, 0.5);

        const int multiplier = 16;
        Repeat::repeatFunc((int)((double)diffEwma/(double)multiplier), multiplier, [this]{
            currentBeat = ++currentBeat%4;
            this->updateLabel(currentBeat);
        });

        flash((int)(0.75 * (double)diffEwma));
    }

    void TempoSynthesizerComponent::flash(int duration) {
        Repeat::repeatFunc(duration/fadeIncrements, fadeIncrements, [this](int i){
            this->updateLabelColour(juce::Colours::white.interpolatedWith(juce::Colours::grey, (float) i / float(fadeIncrements-1)));
        });
    }

    void TempoSynthesizerComponent::updateLabel(uint8_t beat) {
        content = "● " + std::to_string(beat);
        dirty = true;
    }

    void TempoSynthesizerComponent::updateLabelColour(juce::Colour newColour) {
        colour = newColour;
        dirty = true;
    }

    double TempoSynthesizerComponent::ewma(double current, double nextValue, double alpha) {
        return alpha * nextValue + (1 - alpha) * current;
    }
}