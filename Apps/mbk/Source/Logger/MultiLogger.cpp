//
// Created by glynh on 10/12/2022.
//

#include "MultiLogger.h"

namespace logger {
MultiLogger::MultiLogger(const std::vector<Logger*>& loggersVec)
    : loggers(loggersVec) {
}

void MultiLogger::debug(const juce::String& message) {
    for (auto& logger: loggers) {
        logger->debug(message);
    }
}
void MultiLogger::info(const juce::String& message) {
    for (auto& logger: loggers) {
        logger->info(message);
    }
}
void MultiLogger::error(const juce::String& message) {
    for (auto& logger: loggers) {
        logger->error(message);
    }
}
} // namespace logger