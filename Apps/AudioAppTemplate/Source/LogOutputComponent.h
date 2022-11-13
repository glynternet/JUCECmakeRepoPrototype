#pragma once

#include "CommonHeader.h"
#include "Logger.h"
#include <cmath>
#include <ctime>

namespace AudioApp
{
    struct leveledMessage {
        std::string level;
        std::string message;
    };

    class LogOutputComponent : public juce::Component, juce::Timer, public Logger {
    public:
        LogOutputComponent();

        void paint(Graphics& g) override;
        void resized() override;
        void debug(const String &message) override;
        void info(const String &message) override;
        void error(const String &message) override;
        void timerCallback() override;

    private:
        void log(const leveledMessage&);

        juce::TextButton pauseButton;
        bool playing = true;

        juce::TextButton levelButton;
        bool debugLevel = false;

        juce::Label label;
        // TODO: probably a good idea to put some locking on here as I'm not really sure how safe this is
        std::vector<std::string> logMessages;
        std::string content;
        std::atomic<bool> dirty;

        // Used to get time for prepending to log lines
        time_t now;
        tm ltm;
    };
}