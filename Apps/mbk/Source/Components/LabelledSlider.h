#pragma once

#include "JuceHeader.h"

namespace Components
{
class LabelledSlider : public Component
{
public:
    explicit LabelledSlider(const String& labelText);
    LabelledSlider(const String& labelText,
                   const float rangeMin,
                   const float rangeMax,
                   const float value,
                   const float setSkewFactorFromMidPoint,
                   const std::function<void(const double value)>& onValueChange);
    LabelledSlider::LabelledSlider(
        const String& labelText,
        const float rangeMin,
        const float rangeMax,
        const float value,
        const std::function<void(const double value)>& onValueChange);
    // TODO(glynternet): why does it say these are not used?
    LabelledSlider(const String& labelText,
                   const float rangeMin,
                   const float rangeMax,
                   const float rangeInterval,
                   const float value,
                   const float setSkewFactorFromMidPoint,
                   const std::function<void(const double value)>& onValueChange);
    LabelledSlider(
        const String& labelText,
        const float rangeMin,
        const float rangeMax,
        const float valueMin,
        const float valueMax,
        const float skewFactor,
        const std::function<void(const double low, const double high)>& onValueChange);
    void resized() override;

    std::function<void()> onValueChange;

private:
    Slider _slider;
    Label _label;
};
} // namespace
