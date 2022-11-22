#pragma once

#include "JuceHeader.h"

class LabelledSlider : public Component {
public:
    explicit LabelledSlider(const String &labelText);
    Slider &getSlider();
    void resized() override;

    std::function<void()> onValueChange;
private:
    Slider _slider;
    Label _label;
};
