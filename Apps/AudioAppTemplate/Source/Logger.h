#pragma once

#include "CommonHeader.h"
#include <cmath>

namespace AudioApp
{
    class Logger
    {
    public:
        virtual void log(const String &message) = 0;

    private:
    };
}
