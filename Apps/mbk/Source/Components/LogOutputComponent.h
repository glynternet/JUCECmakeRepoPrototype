#pragma once

#include <cmath>
#include <ctime>
#include "../Logger/Logger.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace AudioApp
{
    struct leveledMessage {
        std::string level;
        std::string message;
    };

    class LogOutputComponent : public juce::Component, juce::Timer, public Logger {
    public:
        LogOutputComponent();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void debug(const juce::String &message) override;
        void info(const juce::String &message) override;
        void error(const juce::String &message) override;
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
        tm ltm {};
    };
}
