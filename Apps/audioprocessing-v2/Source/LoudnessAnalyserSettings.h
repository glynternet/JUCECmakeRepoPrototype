//
// Created by glynh on 25/11/2022.
//

#ifndef JUCECMAKEREPO_LOUDNESSANALYSERSETTINGSCOMPONENT_H
#define JUCECMAKEREPO_LOUDNESSANALYSERSETTINGSCOMPONENT_H


#include "LabelledSlider.h"
#include "LoudnessAnalyser.h"

class LoudnessAnalyserSettings : public juce::Component {
public:
    explicit LoudnessAnalyserSettings(LoudnessAnalyser &loudnessAnalyser, const float initialProcessRateHz,
                                      const double initialProcessingBandLow, const double initialProcessingBandHigh,
                                      const int movingAverageInitialWindow, const float initialDecayExponent) {
        setupRangeIn(rangeIn, loudnessAnalyser);
        setupProcessingBandSlider(frequencyProcessingBand, loudnessAnalyser, initialProcessingBandLow,
                                  initialProcessingBandHigh);
        setupDecayLength(decayLength, loudnessAnalyser, initialDecayExponent);
        setupMovingAverageSlider(movingAverage, loudnessAnalyser, movingAverageInitialWindow);
        setupProcessingRateSlider(processRateSlider, loudnessAnalyser, initialProcessRateHz);
    }
private:
    void resized() override {
        auto bounds = getLocalBounds().reduced(10);
        if (bounds.getHeight() > 200) {
            bounds = bounds.removeFromTop(200);
        }
        rangeIn.setBounds(bounds.removeFromTop(bounds.getHeight()/5));
        frequencyProcessingBand.setBounds(bounds.removeFromTop(bounds.getHeight()/4));
        decayLength.setBounds(bounds.removeFromTop(bounds.getHeight()/3));
        movingAverage.setBounds(bounds.removeFromTop(bounds.getHeight()/2));
        processRateSlider.setBounds(bounds);
    }

    // TODO(glynternet): do these sliders actually all need to be references?
    void setupRangeIn(LabelledSlider &slider, LoudnessAnalyser &loudnessAnalyser) {
        addAndMakeVisible(&slider);
        auto &innerSlider = slider.getSlider();
        innerSlider.setSliderStyle(Slider::TwoValueHorizontal);
        innerSlider.setRange(-0.1f, 1.1f);
        innerSlider.setMinValue(0.1f);
        innerSlider.setMaxValue(0.8f);
        innerSlider.setSkewFactor(0.3f);  // TODO: change this from a relatively arbitrary value

        slider.onValueChange = [&slider, &loudnessAnalyser]() {
            loudnessAnalyser.valueShaper.setInMin((float) slider.getSlider().getMinValue());
            loudnessAnalyser.valueShaper.setInMax((float) slider.getSlider().getMaxValue());
        };
    }

    void setupProcessingBandSlider(LabelledSlider &slider, LoudnessAnalyser &loudnessAnalyser,
                                   const double initialProcessingBandLow, const double initialProcessingBandHigh) {
        addAndMakeVisible(slider);
        Slider &innerSlider = slider.getSlider();
        innerSlider.setSliderStyle(Slider::TwoValueHorizontal);
        innerSlider.setRange(0.0f, 1.0f);
        innerSlider.setSkewFactor(0.5f);  // TODO: change this from a relatively arbitrary value
        innerSlider.setMinValue(initialProcessingBandLow);
        innerSlider.setMaxValue(initialProcessingBandHigh);
        slider.onValueChange = [&innerSlider, &loudnessAnalyser] {
            loudnessAnalyser.processingBandIndexLow = innerSlider.getMinValue();
            loudnessAnalyser.processingBandIndexHigh = innerSlider.getMaxValue();
        };
    }

    void setupDecayLength(LabelledSlider &slider, LoudnessAnalyser &loudnessAnalyser, const float initialDecayExponent) {
        addAndMakeVisible(&slider);
        auto &innerSlider = slider.getSlider();
        innerSlider.setValue(initialDecayExponent);
        innerSlider.setRange(TailOff::minExponent, TailOff::maxExponent);
        innerSlider.setSkewFactorFromMidPoint(
                jmap(0.90f, TailOff::minExponent, TailOff::maxExponent));
        slider.onValueChange = [&innerSlider, &loudnessAnalyser]() {
            auto exponent = innerSlider.getValue();
            loudnessAnalyser.decayLength.setExponent((float) exponent);
            loudnessAnalyser.decayLength.setExponent((float) exponent);
        };
    }

    void setupMovingAverageSlider(LabelledSlider &slider, LoudnessAnalyser &loudnessAnalyser,
                                  const int movingAverageInitialWindow) {
        addAndMakeVisible(&slider);
        auto &innerSlider = slider.getSlider();
        innerSlider.setValue(movingAverageInitialWindow);
        innerSlider.setRange(1, 7, 1);
        slider.onValueChange = [&innerSlider, &loudnessAnalyser]() {
            loudnessAnalyser.movingAverage.setPeriod((int) innerSlider.getValue());
        };
    }

    void setupProcessingRateSlider(LabelledSlider &slider, LoudnessAnalyser &loudnessAnalyser,
                                   const float initialProcessRateHz) {
        addAndMakeVisible(&processRateSlider);
        Slider &innerSlider = processRateSlider.getSlider();
        innerSlider.setRange(5, 70);
        innerSlider.setValue(initialProcessRateHz, dontSendNotification);
        // TODO(glynternet): do we need to stop the old timer or does doing this just change the frequency?
        slider.onValueChange = [&innerSlider, &loudnessAnalyser]() {
            loudnessAnalyser.setProcessRateHz((int) innerSlider.getValue());
        };
    }

    LabelledSlider rangeIn{"Range In"};
    LabelledSlider frequencyProcessingBand{"Frequency Band"};
    LabelledSlider decayLength{"Decay Length"};
    LabelledSlider movingAverage{"Window Size"};
    LabelledSlider processRateSlider{"Process rate"};
};


#endif //JUCECMAKEREPO_LOUDNESSANALYSERSETTINGSCOMPONENT_H
