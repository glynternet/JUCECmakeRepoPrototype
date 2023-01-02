#pragma once

#include "JuceHeader.h"

namespace Components
{
class LabelledSlider : public Component
{
public:
    // Simple LabelledSlider with default settings.
    explicit LabelledSlider(const String& labelText);

    // LabelledSlider with range min and max, and initial value of slider.
    LabelledSlider(
        const String& labelText,
        float rangeMin,
        float rangeMax,
        float value,
        const std::function<void(const double value)>& onValueChange);

    // LabelledSlider with skew factor set by setting the midpoint of the slider.
    LabelledSlider(const String& labelText,
                   float rangeMin,
                   float rangeMax,
                   float value,
                   float midpoint,
                   const std::function<void(const double value)>& onValueChange);

    // LabelledSlider with discrete possible values separated by rangeInterval.
    LabelledSlider(const String& labelText,
                   float rangeMin,
                   float rangeMax,
                   float rangeInterval,
                   float value,
                   float midpoint,
                   const std::function<void(const double value)>& onValueChange);

    // LabelledSlider with high and low values, and manually set skew factor.
    LabelledSlider(
        const String& labelText,
        float rangeMin,
        float rangeMax,
        float valueLow,
        float valueHigh,
        float skewFactor,
        const std::function<void(const double low, const double high)>& onValueChange);

    void resized() override;

    std::function<void()> onValueChange;

private:
    Slider _slider;
    Label _label;
};
} // namespace Components
