#pragma once

#include "Logger.h"

namespace logger
{

class MultiLogger : public Logger
{
public:
    explicit MultiLogger(const std::vector<Logger*>& loggers);
    void debug(const juce::String& message) override;
    void info(const juce::String& message) override;
    void error(const juce::String& message) override;

private:
    std::vector<Logger*> _loggers;
};

} // namespace logger
