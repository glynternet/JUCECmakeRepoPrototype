#include "LogOutputComponent.h"
#include <iterator>

namespace AudioApp
{
    LogOutputComponent::LogOutputComponent() : pauseButton("pause"), dirty(true), now(time(nullptr)) {
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
        pauseButton.setBounds(getLocalBounds().removeFromTop(15).removeFromRight(25));
    }

    void LogOutputComponent::info(const String& message) {
        log(leveledMessage{"INFO ", message.toStdString() });
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