#include "TempoAnalyserComponent.h"

namespace AudioApp
{
    static const int fadeIncrements = 8;

    TempoAnalyserComponent::TempoAnalyserComponent() {
        label.setColour (juce::Label::textColourId, colour);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        startTimerHz(30);
    }

    void TempoAnalyserComponent::paint(Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        label.setColour (juce::Label::textColourId, colour);
        label.setText(content, juce::dontSendNotification);
    }

    void TempoAnalyserComponent::resized() {
        label.setBounds(getLocalBounds());
    }

    void TempoAnalyserComponent::timerCallback() {
        if (dirty.exchange(false)) {
            repaint();
        }
    }

    void TempoAnalyserComponent::processAudioFrame (double* frame) {
        btrack.processAudioFrame(frame);
        if (btrack.beatDueInCurrentFrame()) {
            // TODO: move this out of the tempo analysis, we should report raw value here and do averaging in the tempo synthesizer.
            beat();
            if (onBeat != nullptr) {
                onBeat(diffEwma);
            }
        }
    }

    void TempoAnalyserComponent::updateSamplePerBlockExpected(int samplePerBlockExpected){
        btrack.updateHopAndFrameSize(samplePerBlockExpected / 2, samplePerBlockExpected);
    }

    void TempoAnalyserComponent::beat() {
        auto tempo = btrack.getCurrentTempoEstimate();
        auto current = juce::Time::currentTimeMillis();
        auto diff = current - lastTime;
        diffEwma = ewma(diffEwma, (double)diff, 0.5);
        lastTime = current;

        // this might actually be better as a function that says "repeat X time in the next Y milliseconds@
        setLabelColour(juce::Colours::white);
        for (int i = 0; i < fadeIncrements; ++i) {
            const double proportion = (float) i / float(fadeIncrements);
            // beatDueInCurrentFrame only happens every other beat and we want to fade over 2 beats, so we do
            // 1500 * seconds per beat to take 75% of the time between flashes to fade.
            // * 1000 to convert seconds to milliseconds
            // * 0.75 to convert to 75% of the time between "beats"
            // * 2 because the beat detection happens every other beat (there may be something in the research paper that mentions why this is)
            juce::Timer::callAfterDelay((int)((double)diffEwma * 0.75 * proportion), [this, proportion]{
                this->setLabelColour(juce::Colours::white.interpolatedWith(juce::Colours::grey, (float)proportion));
            });
        }

        double tempoFromManualCalculation = 120000. / (double) diff;
        double tempoFromEWMA = 120000. / (double) diffEwma;
        ++beats;
        currentBeat = ++currentBeat%4;
        setLabelText(tempo, tempoFromManualCalculation, tempoFromEWMA);
    }

    void TempoAnalyserComponent::setLabelColour(juce::Colour newColour) {
        colour = newColour;
        dirty = true;
    }

    void TempoAnalyserComponent::setLabelText(double btrackTempo, double manualCalcTempo, double movingAverageTempo) {
        content = "● " +
                  std::to_string(currentBeat) + " " +
                  std::to_string(btrackTempo) + " " +
                  std::to_string(diffEwma) + " " +
                  std::to_string(manualCalcTempo) + " " +
                  std::to_string(movingAverageTempo);
        dirty = true;
    }

    void TempoAnalyserComponent::repeatFunc(int interval, int count, const std::function<void()>& call) {
        // TODO: use a single timer here instead
        for (int i = 0; i < count; ++i) {
            juce::Timer::callAfterDelay((1+i)*interval, call);
        }
    }

    double TempoAnalyserComponent::ewma(double current, double nextValue, double alpha) {
        return alpha * nextValue + (1 - alpha) * current;
    }
}