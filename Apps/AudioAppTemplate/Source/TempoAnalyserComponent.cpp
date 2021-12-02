#include "TempoAnalyserComponent.h"

namespace AudioApp
{
    static const int fadeIncrements = 8;

    TempoAnalyserComponent::TempoAnalyserComponent() {
        tempoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
        tempoLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(tempoLabel);
    }

    void TempoAnalyserComponent::paint(Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void TempoAnalyserComponent::resized() {
        tempoLabel.setBounds(getLocalBounds());
    }

    void TempoAnalyserComponent::processAudioFrame (double* frame) {
        btrack.processAudioFrame(frame);
        if (btrack.beatDueInCurrentFrame()) {
            if (onBeat != nullptr) {
                onBeat();
            }
            beat();
        }
    }

    void TempoAnalyserComponent::updateSamplePerBlockExpected(int samplePerBlockExpected){
        btrack.updateHopAndFrameSize(samplePerBlockExpected / 2, samplePerBlockExpected);
    }

    void TempoAnalyserComponent::beat() {
        tempoLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        auto tempo = btrack.getCurrentTempoEstimate();
        auto current = juce::Time::currentTimeMillis();
        auto diff = current - lastTime;
        diffEwma = ewma(diffEwma, (double)diff, 0.1);
        lastTime = current;

        // this might actually be better as a function that says "repeat X time in the next Y milliseconds@
        for (int i = 0; i < fadeIncrements; ++i) {
            const double proportion = (float) i / float(fadeIncrements);
            // beatDueInCurrentFrame only happens every other beat and we want to fade over 2 beats, so we do
            // 1500 * seconds per beat to take 75% of the time between flashes to fade.
            // * 1000 to convert seconds to milliseconds
            // * 0.75 to convert to 75% of the time between "beats"
            // * 2 because the beat detection happens every other beat (there may be something in the research paper that mentions why this is)
            juce::Timer::callAfterDelay((int)((double)diffEwma * 0.75 * proportion), [this, proportion]{
                this->tempoLabel.setColour (juce::Label::textColourId, juce::Colours::white.interpolatedWith(juce::Colours::lightgrey, (float)proportion));
            });
        }

        double tempoFromManualCalculation = 120000. / (double) diff;
        double tempoFromEWMA = 120000. / (double) diffEwma;
        ++beats;
        currentBeat = ++currentBeat%4;
        const int multiplier = 16;
        repeatFunc((int)(diffEwma/(double)multiplier), multiplier-1, [this, tempoFromManualCalculation, tempo, tempoFromEWMA]{
            currentBeat = ++currentBeat%4;
            tempoLabel.setText(
                    std::to_string(beats) + " " +
                    std::to_string(currentBeat) + " " +
                    std::to_string(tempo) + " " +
                    std::to_string(diffEwma) + " " +
                    std::to_string(tempoFromManualCalculation) + " " +
                    std::to_string(tempoFromEWMA),
                    juce::dontSendNotification);
        });

        tempoLabel.setText(
                std::to_string(beats) + " " +
                std::to_string(currentBeat) + " " +
                std::to_string(tempo) + " " +
                std::to_string(diffEwma) + " " +
                std::to_string(tempoFromManualCalculation) + " " +
                std::to_string(tempoFromEWMA),
                juce::dontSendNotification);
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