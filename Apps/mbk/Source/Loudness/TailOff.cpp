//
// Created by glynh on 22/11/2022.
//

#include "TailOff.h"

namespace Loudness {

// maxDecayCoefficient defines the maxiumum that the value given to TailOff can decay
// when passed to getValue.
// A coefficient of 0.9 means that the value returned from getValue will be the maximum
// of 0.9*previousValue and the value passed in itself.
//   e.g.
//   With a TailOff with maxDecayCoefficient of 0.9, if the previous value stored in the
//   TailOff is 1.0, then:
//     * getValue(2.0) would return 2.0
//     * getValue(0.5) would return 0.9
TailOff::TailOff(float maxDecayCoefficient) {
    setMaxDecayCoefficient(maxDecayCoefficient); }

void TailOff::setMaxDecayCoefficient(float exponent) {
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
}
