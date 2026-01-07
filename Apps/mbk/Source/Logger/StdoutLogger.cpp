//
// Created by glynh on 23/11/2022.
//

#include "StdoutLogger.h"

namespace logger {
StdoutLogger::StdoutLogger()
    : StdoutLogger(true) {
}
StdoutLogger::StdoutLogger(bool debug)
    : debugEnabled(debug) {
}

void StdoutLogger::debug(const juce::String& message) {
    if (this->debugEnabled) {
        std::cout << "DEBUG: " << message.toStdString() << '\n';
    }
}

void StdoutLogger::info(const juce::String& message) {
    std::cout << "INFO : " << message.toStdString() << '\n';
}
void StdoutLogger::error(const juce::String& message) {
    std::cout << "ERROR: " << message.toStdString() << '\n';
}
} // namespace logger