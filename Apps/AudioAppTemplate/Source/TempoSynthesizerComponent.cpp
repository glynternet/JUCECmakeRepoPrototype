#include "TempoSynthesizerComponent.h"
#include "LogOutputComponent.h"

namespace AudioApp
{
    const int multiplier = 4;
    static const int fadeIncrements = 8;

    TempoSynthesizerComponent::TempoSynthesizerComponent() {
        label.setColour (juce::Label::textColourId, colour);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        juce::HighResolutionTimer::startTimer(1);
        juce::Timer::startTimerHz(60);
    }

    void TempoSynthesizerComponent::paint(Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        g.setColour(colour);
        auto segmentWidth = getLocalBounds().getWidth()/multiplier;
        g.fillRect(currentBeat * segmentWidth,0,segmentWidth,getLocalBounds().getHeight());
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

        for (uint8_t i = 1; i < multiplier; ++i) {
            scheduledBeats.push_back(scheduledBeat{
                timeOfBeat + (diffEwma / (double) multiplier * (double)i),
                i,
            });
        }

        flash(0.75f * (float)diffEwma);
    }

    void TempoSynthesizerComponent::flash(float duration) {
        Repeat::repeatFunc(duration/fadeIncrements, fadeIncrements, [this](int i){
            this->updateLabelColour(juce::Colours::white.interpolatedWith(juce::Colours::grey, (float) i / float(fadeIncrements-1)));
        });
    }

    void TempoSynthesizerComponent::updateBeat(uint8_t beat) {
        currentBeat = beat;
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