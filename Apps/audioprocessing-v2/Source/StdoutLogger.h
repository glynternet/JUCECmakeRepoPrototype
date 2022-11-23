//
// Created by glynh on 23/11/2022.
//

#ifndef JUCECMAKEREPO_STDOUTLOGGER_H
#define JUCECMAKEREPO_STDOUTLOGGER_H

#include "../../mbk/Source/Logger.h"

class StdoutLogger : AudioApp::Logger {
    void debug(const juce::String &message);
    void info(const juce::String &message);
    void error(const juce::String &message);
};

#endif //JUCECMAKEREPO_STDOUTLOGGER_H
