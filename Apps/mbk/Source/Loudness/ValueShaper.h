#include "JuceHeader.h"

namespace Loudness
{
class ValueShaper
{
public:
    ValueShaper(float inMin, float inMax, float outMin, float outMax)
        : _inMin(inMin)
        , _inMax(inMax)
        , _outMin(outMin)
        , _outMax(outMax)
    {
    }
    ~ValueShaper() = default;

    void setInMin(float value) { _inMin = value; }
    void setInMax(float value) { _inMax = value; }

    [[nodiscard]] float shape(float value) const
    {
        // TODO: allow configuration of this from the GUI in some advanced settings
        return jlimit(0.0f, 1.0f, jmap(value, _inMin, _inMax, _outMin, _outMax));
    }

private:
    float _inMin, _inMax;
    float _outMin, _outMax;
};
} // namespace Loudness
