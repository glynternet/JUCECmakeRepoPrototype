#include "LogOutputComponent.h"

namespace AudioApp
{
    LogOutputComponent::LogOutputComponent()
    {
        message.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        message.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(message);
    }

    void LogOutputComponent::paint(Graphics& g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void LogOutputComponent::resized() {
        message.setBounds(getLocalBounds());
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

        this->message.setText(joined.str(), juce::dontSendNotification);
    }
}