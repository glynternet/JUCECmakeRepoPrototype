#include "LogOutputComponent.h"
#include <iterator>

namespace AudioApp
{
    LogOutputComponent::LogOutputComponent() : pauseButton("pause"), levelButton("debug"), dirty(true), now(time(nullptr)) {
        // TODO: set to monospace font
        label.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        label.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(label);

        pauseButton.onClick = [this] {
            this->playing = !this->playing;
            if (this->playing) {
                this->pauseButton.setButtonText("pause");
                dirty = true;
            } else {
                this->pauseButton.setButtonText("play");
            }
        };
        addAndMakeVisible(pauseButton);

        levelButton.onClick = [this] {
            this->debugLevel = !this->debugLevel;
            if (this->debugLevel) {
                this->levelButton.setButtonText("info");
            } else {
                this->levelButton.setButtonText("debug");
            }
            dirty = true;
        };
        addAndMakeVisible(levelButton);
        startTimerHz(30);
    }

    void LogOutputComponent::timerCallback() {
        if (dirty.exchange(false)) {
            repaint();
        }
    }

    void LogOutputComponent::paint(Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        if (playing) {
            label.setText(content, juce::dontSendNotification);
        }
    }


    void LogOutputComponent::resized() {
        label.setBounds(getLocalBounds());
        static const int buttonWidth = 35;
        const juce::Rectangle<int> &pauseBounds = getLocalBounds().removeFromTop(15).removeFromRight(buttonWidth);
        pauseButton.setBounds(pauseBounds);
        levelButton.setBounds(pauseBounds.translated(-buttonWidth, 0));
    }

    void LogOutputComponent::debug(const String& message) {
        if (this->debugLevel)
            log(leveledMessage{"DEBUG", message.toStdString() });
    }

    void LogOutputComponent::info(const String& message) {
        log(leveledMessage{"INFO", message.toStdString() });
    }

    void LogOutputComponent::error(const String& message) {
        log(leveledMessage{"ERROR", message.toStdString() });
    }

    void LogOutputComponent::log(const leveledMessage& message) {
        if (logMessages.size() > 100) {
            logMessages.pop_back();
        }

        localtime_s(&ltm,&now);

        std::ostringstream oss;
        oss << std::put_time(&ltm, "%H:%M:%S");
        oss << " ";
        oss << message.level;
        oss << " ";
        oss << message.message;
        logMessages.insert(logMessages.begin(),oss.str());

        const char* const delim = "\n";

        std::ostringstream joined;
        std::copy(logMessages.begin(), logMessages.end(),
                  std::ostream_iterator<std::string>(joined, delim));

        content = joined.str();
        dirty = true;
    }
}