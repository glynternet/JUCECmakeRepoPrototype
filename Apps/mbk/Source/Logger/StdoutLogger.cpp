//
// Created by glynh on 23/11/2022.
//

#include "StdoutLogger.h"

namespace logger
{
StdoutLogger::StdoutLogger(): StdoutLogger(true) {}
StdoutLogger::StdoutLogger(bool debug)
{
    _debug = debug;
}

void StdoutLogger::debug(const juce::String& message)
{
    if (this->_debug)
    {
        std::cout << "DEBUG: " << message.toStdString() << std::endl;
    }
}

void StdoutLogger::info(const juce::String& message)
{
    std::cout << "INFO : " << message.toStdString() << std::endl;
}
void StdoutLogger::error(const juce::String& message)
{
    std::cout << "ERROR: " << message.toStdString() << std::endl;
}
} // namespace Logger