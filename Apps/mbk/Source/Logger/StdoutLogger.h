//
// Created by glynh on 23/11/2022.
//

#ifndef JUCECMAKEREPO_STDOUTLOGGER_H
#define JUCECMAKEREPO_STDOUTLOGGER_H

#include "Logger.h"

class StdoutLogger : public AudioApp::Logger {
public:
    StdoutLogger();
    explicit StdoutLogger(bool debugMode);
    void debug(const juce::String &message) override;
    void info(const juce::String &message) override;
    void error(const juce::String &message) override;
    bool test;
private:
    bool _debug;
};

#endif //JUCECMAKEREPO_STDOUTLOGGER_H
