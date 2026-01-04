//
// Created by glynh on 22/11/2022.
//
#include "LabelledSlider.h"

namespace Components {
LabelledSlider::LabelledSlider(const String& labelText) {
    addAndMakeVisible(_slider);
    _slider.onValueChange = [this]() { onValueChange(); };
    _slider.setTextBoxStyle(Slider::NoTextBox, false, 160, _slider.getTextBoxHeight());

    addAndMakeVisible(_label);
    _label.setText(labelText, dontSendNotification);
    _label.attachToComponent(&_slider, true);
}

LabelledSlider::LabelledSlider(const String& labelText,
                               const float rangeMin,
                               const float rangeMax,
                               const float value,
                               std::function<void(double value)> onValueChange)
    : LabelledSlider(labelText) {
    _slider.setRange(rangeMin, rangeMax);
    _slider.setValue(value);
    _slider.onValueChange = [this, onValueChange = std::move(onValueChange)]() {
        onValueChange(this->_slider.getValue());
    };
}

LabelledSlider::LabelledSlider(const String& labelText,
                               const float rangeMin,
                               const float rangeMax,
                               const float value,
                               const float midpoint,
                               std::function<void(double value)> onValueChange)
    : LabelledSlider(labelText) {
    _slider.setRange(rangeMin, rangeMax);
    _slider.setSkewFactorFromMidPoint(midpoint);
    _slider.setValue(value);
    _slider.onValueChange = [this, onValueChange = std::move(onValueChange)]() {
        onValueChange(this->_slider.getValue());
    };
}

LabelledSlider::LabelledSlider(const String& labelText,
                               const float rangeMin,
                               const float rangeMax,
                               const float rangeInterval,
                               const float value,
                               const float midpoint,
                               std::function<void(double value)> onValueChange)
    : LabelledSlider(labelText) {
    _slider.setRange(rangeMin, rangeMax, rangeInterval);
    _slider.setValue(value);
    _slider.setSkewFactor(midpoint);
    _slider.onValueChange = [this, onValueChange = std::move(onValueChange)]() {
        onValueChange(this->_slider.getValue());
    };
}

LabelledSlider::LabelledSlider(const String& labelText,
                               const float rangeMin,
                               const float rangeMax,
                               const float valueLow,
                               const float valueHigh,
                               const float skewFactor,
                               std::function<void(double low, double high)> onValueChange)
    : LabelledSlider(labelText) {
    _slider.setSliderStyle(Slider::TwoValueHorizontal);
    _slider.setRange(rangeMin, rangeMax);
    _slider.setSkewFactor(skewFactor);
    _slider.setMinValue(valueLow);
    _slider.setMaxValue(valueHigh);
    _slider.onValueChange = [this, onValueChange = std::move(onValueChange)]() {
        onValueChange(this->_slider.getMinValue(), this->_slider.getMaxValue());
    };
}

LabelledSlider::LabelledSlider(const String& labelText,
                               const int rangeMin,
                               const int rangeMax,
                               const int valueLow,
                               const int valueHigh,
                               std::function<void(int low, int high)> onValueChange,
                               std::function<String(int low, int high)> formatLabel)
    : _formatLabel(std::move(formatLabel))
    , _hasValueLabel(true) {
    addAndMakeVisible(_slider);
    _slider.setTextBoxStyle(Slider::NoTextBox, false, 160, _slider.getTextBoxHeight());

    addAndMakeVisible(_label);
    _label.setText(labelText, dontSendNotification);
    _label.attachToComponent(&_slider, true);

    _slider.setSliderStyle(Slider::TwoValueHorizontal);
    _slider.setRange(rangeMin, rangeMax, 1.0); // interval of 1 for discrete steps
    _slider.setMinValue(valueLow);
    _slider.setMaxValue(valueHigh);

    _valueLabel.setJustificationType(Justification::centredLeft);
    _valueLabel.setFont(Font(12.0f));
    addAndMakeVisible(_valueLabel);
    updateValueLabel();

    _slider.onValueChange = [this, onValueChange = std::move(onValueChange)]() {
        int low = static_cast<int>(this->_slider.getMinValue());
        int high = static_cast<int>(this->_slider.getMaxValue());
        onValueChange(low, high);
        updateValueLabel();
    };
}

void LabelledSlider::updateValueLabel() {
    if (_hasValueLabel && _formatLabel) {
        int low = static_cast<int>(_slider.getMinValue());
        int high = static_cast<int>(_slider.getMaxValue());
        _valueLabel.setText(_formatLabel(low, high), dontSendNotification);
    }
}

void LabelledSlider::resized() {
    const int labelWidth(90);
    _slider.setBounds(labelWidth, 0, getWidth() - labelWidth, 20);
    if (_hasValueLabel) {
        _valueLabel.setBounds(labelWidth, 20, getWidth() - labelWidth, 16);
    }
}
} // namespace Components
