#pragma once

#include "Logger.h"

namespace logger
{
class StdoutLogger : public Logger
{
public:
    StdoutLogger();
    explicit StdoutLogger(bool debugMode);
    void debug(const juce::String& message) override;
    void info(const juce::String& message) override;
    void error(const juce::String& message) override;
    bool test = false;

private:
    bool debugEnabled = false;
};
} // namespace logger
