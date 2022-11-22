//
// Created by glynh on 22/11/2022.
//

#include "TailOff.h"

// TODO: is exponent the right name here?
TailOff::TailOff(float exponent) { setExponent(exponent); }

float TailOff::getExponent() { return _exponent; }

void TailOff::setExponent(float exponent) {
    if (exponent > maxExponent) {
        _exponent = maxExponent;
        // TODO: throw something?
        return;
    }

    if (exponent < minExponent) {
        _exponent = minExponent;
        // TODO: throw something?
        return;
    }

    _exponent = exponent;
}

float TailOff::getValue(float in) {
    float min = _previousValue * _exponent;
    _previousValue = fmax(in, min);
    return _previousValue;
}
