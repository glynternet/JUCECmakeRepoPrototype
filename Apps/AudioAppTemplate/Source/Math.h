//
// Created by glynh on 26/03/2022.
//

#ifndef JUCECMAKEREPO_MATH_H
#define JUCECMAKEREPO_MATH_H

double ewma(double current, double nextValue, double alpha) {
    return alpha * nextValue + (1 - alpha) * current;
}

int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

#endif //JUCECMAKEREPO_MATH_H
