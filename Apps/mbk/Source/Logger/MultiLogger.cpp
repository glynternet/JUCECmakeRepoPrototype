//
// Created by glynh on 10/12/2022.
//

#include "MultiLogger.h"

namespace logger
{
MultiLogger::MultiLogger(const std::vector<Logger*>& loggers){
    _loggers = loggers;
}
void MultiLogger::debug(const juce::String &message) {
    for (auto& _logger: _loggers) {
        _logger->debug(message);
    }
}
void MultiLogger::info(const juce::String &message) {
    for (auto& _logger: _loggers) {
        _logger->info(message);
    }
}
void MultiLogger::error(const juce::String &message) {
    for (auto& _logger: _loggers) {
        _logger->error(message);
    }
}
} // namespace AudioApp