//
// Created by glyn on 11/12/2021.
//

#include "OSCComponent.h"

namespace AudioApp {
    static const int OSCPort = 9000;
    static const std::string OSCPortString = std::to_string(OSCPort);

    OSCComponent::OSCComponent(Logger &l) : logger(l) {
        targetAddress.setJustificationType(juce::Justification::centred);
        targetAddress.setEditable(true, false, true);
        targetAddress.onEditorShow = [this] {
            connectOSCButton.setEnabled(false);
        };
        targetAddress.onEditorHide = [this] {
            connectOSCButton.setEnabled(targetAddress.getText().length() > 0);
        };
        connectOSCButton.onClick = [this] {
            if (senderConnected) {
                disconnectOSCSender();
            } else {
                connectOSCSender(targetAddress.getText(true));
            }
        };
        setSenderConnectedState(false);
        addAndMakeVisible(connectOSCButton);
        addAndMakeVisible(targetAddress);
    }

    void OSCComponent::resized() {
        auto bounds = getLocalBounds().reduced(10);
        connectOSCButton.setBounds(bounds.removeFromRight(bounds.proportionOfWidth(0.5f)));
        targetAddress.setBounds(bounds);
    }

    void OSCComponent::paint(juce::Graphics &) {}

    bool OSCComponent::send(const juce::OSCMessage &message) {
        if (senderConnected) {
            try {
                if (sender.send(message)) {
                    logger.debug("Message sent");
                    return true;
                }
                logger.error("Error sending message");
                return false;
            }
            catch (const juce::OSCException &e) {
                logger.error("Error sending message: " + e.description);
                return false;
            }
        } else {
            // TODO(glynternet): rate limit this specific message
            logger.debug("Sender not connected");
            return false;
        }
    }

    void OSCComponent::connectOSCSender(const juce::String &address) {
        auto target = address + ":" + OSCPortString;
        auto success = sender.connect(address, OSCPort);
        if (!success) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Connection error",
                                             "Error: could not connect to UDP port 9000.", "OK");
            logger.error("Error connecting OSC with target " + target);
            return;
        }
        setSenderConnectedState(true);
        logger.info("Connected OSC with target " + target);
    }

    void OSCComponent::disconnectOSCSender() {
        auto success = sender.disconnect();
        if (!success) {
            logger.error("Error disconnecting OSC");
            return;
        }
        setSenderConnectedState(false);
        logger.info("Disconnected OSC");
    }

    void OSCComponent::setSenderConnectedState(bool connected) {
        senderConnected = connected;
        if (senderConnected) {
            connectOSCButton.setButtonText("Disconnect OSC");
            connectOSCButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
        } else {
            connectOSCButton.setButtonText("Connect OSC");
            connectOSCButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
        }
    }
}