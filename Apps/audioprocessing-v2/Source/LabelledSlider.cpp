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

Slider &LabelledSlider::getSlider() { return _slider; }

void LabelledSlider::resized() {
    const int labelWidth(80);
    _slider.setBounds(labelWidth, 0, getWidth() - labelWidth, 20);
}