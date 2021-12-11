#pragma once

#include "CommonHeader.h"
#include "Logger.h"
#include <cmath>

namespace AudioApp
{
    struct leveledMessage {
        std::string level;
        std::string message;
    };

    class LogOutputComponent : public juce::Component, juce::Timer, public Logger
    {
    public:
        LogOutputComponent();

        void paint(Graphics& g) override;
        void resized() override;
        void info(const String &message);
        void error(const String &message);
        void timerCallback() override;

    private:
        void log(leveledMessage);

        juce::Label label;
        // TODO: probably a good idea to put some locking on here as I'm not really sure how safe this is
        std::vector<std::string> logMessages;
        std::string content;
        std::atomic<bool> dirty;
    };
}
