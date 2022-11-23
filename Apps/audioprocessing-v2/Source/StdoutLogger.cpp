//
// Created by glynh on 23/11/2022.
//

#include "StdoutLogger.h"

void StdoutLogger::debug(const juce::String &message) {
    std::cout << "DEBUG: " << message.toStdString() << std::endl;
}

void StdoutLogger::info(const juce::String &message) {
    std::cout << "INFO : " << message.toStdString() << std::endl;
}

void StdoutLogger::error(const juce::String &message) {
    std::cout << "ERROR: " << message.toStdString() << std::endl;
}

