#pragma once

#include <cmath>

namespace Loudness
{
class TailOff
{
public:
    explicit TailOff(float);

    float getValue(float);

    void setMaxDecayCoefficient(float);

    static constexpr float maxExponent = 0.9999F;
    static constexpr float minExponent = 0.0F;

private:
    float exponent = 0.0F;
    float previousValue = 0.0F;
};
} // namespace Loudness
