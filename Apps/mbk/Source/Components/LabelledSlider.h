#pragma once

#include "JuceHeader.h"

namespace Components {
class LabelledSlider : public Component {
public:
    // Simple LabelledSlider with default settings.
    explicit LabelledSlider(const String& labelText);

    // LabelledSlider with range min and max, and initial value of slider.
    LabelledSlider(const String& labelText,
                   float rangeMin,
                   float rangeMax,
                   float value,
                   std::function<void(double value)> onValueChange);

    // LabelledSlider with skew factor set by setting the midpoint of the slider.
    LabelledSlider(const String& labelText,
                   float rangeMin,
                   float rangeMax,
                   float value,
                   float midpoint,
                   std::function<void(double value)> onValueChange);

    // LabelledSlider with discrete possible values separated by rangeInterval.
    LabelledSlider(const String& labelText,
                   float rangeMin,
                   float rangeMax,
                   float rangeInterval,
                   float value,
                   float midpoint,
                   std::function<void(double value)> onValueChange);

    // LabelledSlider with high and low values, and manually set skew factor.
    LabelledSlider(const String& labelText,
                   float rangeMin,
                   float rangeMax,
                   float valueLow,
                   float valueHigh,
                   float skewFactor,
                   std::function<void(double low, double high)> onValueChange);

    // Two-thumb indexed slider with discrete steps and value label display.
    // Used for bin selection where each step represents an integer index.
    LabelledSlider(const String& labelText,
                   int rangeMin,
                   int rangeMax,
                   int valueLow,
                   int valueHigh,
                   std::function<void(int low, int high)> onValueChange,
                   std::function<String(int low, int high)> formatLabel);

    // Two-thumb float slider with value label display.
    LabelledSlider(const String& labelText,
                   float rangeMin,
                   float rangeMax,
                   float valueLow,
                   float valueHigh,
                   float skewFactor,
                   std::function<void(double low, double high)> onValueChange,
                   std::function<String(double low, double high)> formatLabel);

    void resized() override;

    /** Update the value label text (call when external state like sample rate changes) */
    void updateValueLabel();

    /** Set the minimum value of a two-value slider without triggering callback */
    void setMinValue(double value, NotificationType notification = dontSendNotification);

    /** Set the maximum value of a two-value slider without triggering callback */
    void setMaxValue(double value, NotificationType notification = dontSendNotification);

    std::function<void()> onValueChange;

private:
    Slider _slider;
    Label _label;
    Label _valueLabel;
    std::function<String(double, double)> _formatLabel;
    bool _hasValueLabel = false;
};
} // namespace Components
