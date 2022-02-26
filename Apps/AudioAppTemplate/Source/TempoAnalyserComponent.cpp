#include "TempoAnalyserComponent.h"

namespace AudioApp
{
    static const int fadeIncrements = 16;

    TempoAnalyserComponent::TempoAnalyserComponent() {
        startTimerHz(60);
        lastTime = juce::Time::getMillisecondCounterHiRes();
    }

    void TempoAnalyserComponent::paint(Graphics& g) {
        g.fillAll (colour);
    }

    void TempoAnalyserComponent::resized() {}

    void TempoAnalyserComponent::timerCallback() {
        if (dirty.exchange(false)) {
            repaint();
        }
    }

    void TempoAnalyserComponent::processAudioFrame (double* frame) {
        btrack.processAudioFrame(frame);
        if (btrack.beatDueInCurrentFrame()) {
            auto current = juce::Time::getMillisecondCounterHiRes();
            auto diff = current - lastTime;
            lastTime = current;
            if (onBeat != nullptr) {
                onBeat(diff);
            }
            flash(0.75f * (float)diff);
        }
    }

    void TempoAnalyserComponent::updateSamplePerBlockExpected(int samplePerBlockExpected){
        btrack.updateHopAndFrameSize(samplePerBlockExpected / 2, samplePerBlockExpected);
    }

    void TempoAnalyserComponent::flash(float duration) {
        Repeat::repeatFunc(duration/fadeIncrements, fadeIncrements, [this](int i){
            this->setColour(juce::Colours::white.interpolatedWith(juce::Colours::grey, (float) i / float(fadeIncrements-1)));
        });
    }

    void TempoAnalyserComponent::setColour(juce::Colour newColour) {
        colour = newColour;
        dirty = true;
    }
}