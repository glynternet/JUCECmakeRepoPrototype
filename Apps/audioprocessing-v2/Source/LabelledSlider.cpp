//
// Created by glynh on 22/11/2022.
//
#include "LabelledSlider.h"

LabelledSlider::LabelledSlider(const String &labelText) {
    addAndMakeVisible(_slider);
    _slider.onValueChange = [this]() { onValueChange(); };
    _slider.setTextBoxStyle(Slider::NoTextBox, false, 160, _slider.getTextBoxHeight());

    addAndMakeVisible(_label);
    _label.setText(labelText, dontSendNotification);
    _label.attachToComponent(&_slider, true);
}

LabelledSlider::LabelledSlider(const String &labelText, const float rangeMin, const float rangeMax,
                               const float value, const std::function<void(const double value)>& onValueChange) : LabelledSlider(labelText) {
    _slider.setRange(rangeMin, rangeMax);
    _slider.setValue(value);
    _slider.onValueChange = [this, onValueChange]() {
        onValueChange(this->_slider.getValue());
    };
}

LabelledSlider::LabelledSlider(const String &labelText, const float rangeMin, const float rangeMax,
                               const float value, const float setSkewFactorFromMidPoint,
                               const std::function<void(const double value)>& onValueChange) : LabelledSlider(labelText) {
    _slider.setRange(rangeMin, rangeMax);
    _slider.setSkewFactorFromMidPoint(setSkewFactorFromMidPoint);
    _slider.setValue(value);
    _slider.onValueChange = [this, onValueChange]() {
        onValueChange(this->_slider.getValue());
    };
}

LabelledSlider::LabelledSlider(const String &labelText,
                               const float rangeMin, const float rangeMax, const float rangeInterval,
                               const float value, const float skewFactor,
                               const std::function<void(const double value)>& onValueChange) : LabelledSlider(labelText) {
    _slider.setRange(rangeMin, rangeMax, rangeInterval);
    _slider.setValue(value);
    _slider.setSkewFactor(skewFactor);
    _slider.onValueChange = [this, onValueChange]() {
        onValueChange(this->_slider.getValue());
    };
}

LabelledSlider::LabelledSlider(const String &labelText, const float rangeMin, const float rangeMax,
                               const float valueMin, const float valueMax, const float skewFactor,
                               const std::function<void(const double low, const double high)>& onValueChange) : LabelledSlider(
        labelText) {
    _slider.setSliderStyle(Slider::TwoValueHorizontal);
    _slider.setRange(rangeMin, rangeMax);
    _slider.setSkewFactor(skewFactor);
    _slider.setMinValue(valueMin);
    _slider.setMaxValue(valueMax);
    _slider.onValueChange = [this, onValueChange](){
        onValueChange(this->_slider.getMinValue(), this->_slider.getMaxValue());
    };
}

Slider &LabelledSlider::getSlider() { return _slider; }

void LabelledSlider::resized() {
    const int labelWidth(90);
    _slider.setBounds(labelWidth, 0, getWidth() - labelWidth, 20);
}
