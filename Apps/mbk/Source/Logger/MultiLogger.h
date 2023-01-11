//
// Created by glynh on 10/12/2022.
//

#ifndef JUCECMAKEREPO_MULTILOGGER_H
#define JUCECMAKEREPO_MULTILOGGER_H

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

} // namespace AudioApp

#endif //JUCECMAKEREPO_MULTILOGGER_H
