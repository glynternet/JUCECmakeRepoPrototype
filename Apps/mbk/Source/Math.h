#pragma once

inline double ewma(double current, double nextValue, double alpha) {
    return alpha * nextValue + (1 - alpha) * current;
}

inline int ipow(int base, int exp)
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
