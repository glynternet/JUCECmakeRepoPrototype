#pragma once

#include <math.h>

class TailOff {
public:
    explicit TailOff(float);

    float getValue(float);

    float getExponent();

    void setExponent(float);

    static constexpr float maxExponent = 0.9999f;
    static constexpr float minExponent = 0.0f;

private:
    float _exponent;
    float _previousValue;
};

