#include "LogOutputComponent.h"
#include <iterator>

namespace AudioApp
{
    LogOutputComponent::LogOutputComponent()
    {
        label.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        label.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(label);
        startTimerHz(30);
    }

    void LogOutputComponent::timerCallback() {
        if (dirty.exchange(false)) {
            repaint();
        }
    }

    void LogOutputComponent::paint(Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        label.setText(content, juce::dontSendNotification);
    }

    void LogOutputComponent::resized() {
        label.setBounds(getLocalBounds());
    }

    void LogOutputComponent::log(const String& message) {
        if (logMessages.size() > 10) {
            logMessages.pop_back();
        }
        logMessages.insert(logMessages.begin(),  message.toStdString());

        const char* const delim = "\n";

        std::ostringstream joined;
        std::copy(logMessages.begin(), logMessages.end(),
                  std::ostream_iterator<std::string>(joined, delim));

        content = joined.str();
        dirty = true;
    }
}