#pragma once

#include "CommonHeader.h"
#include <cmath>

namespace AudioApp
{
    class Logger
    {
    public:
        virtual void info(const String &message) = 0;
        virtual void error(const String &message) = 0;
    };
}
