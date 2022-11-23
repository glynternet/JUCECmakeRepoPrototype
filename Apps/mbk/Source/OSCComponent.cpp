//
// Created by glyn on 11/12/2021.
//

#include "OSCComponent.h"
#include <memory>

namespace AudioApp {
    static const int OSCPort = 9000;
    static const std::string OSCPortString = std::to_string(OSCPort);

    // the value 123 is provided to create the element in the arguments slice and it's always mutated before any message is sent.
    static const std::unique_ptr<juce::OSCMessage> clockMessage = std::make_unique<juce::OSCMessage>("/clock",
                                                                                                     (juce::String) "millisPerBeat",
                                                                                                     (float) 123);

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
        connectOSCButton.setBounds(bounds.removeFromRight(bounds.getWidth() / 2));
        targetAddress.setBounds(bounds);
    }

    void OSCComponent::paint(Graphics &) {}

    void OSCComponent::sendBeatMessage(double period) {
        if (senderConnected) {
            try {
                // modify period argument of message
                (*clockMessage)[1] = (float) period;
                if (sender.send(*clockMessage)) {
                    logger.debug("Message sent");
                } else {
                    logger.error("Error sending message");
                }
            }
            catch (const juce::OSCException &e) {
                logger.error("Error sending message: " + e.description);
            }
        } else {
            logger.debug("Sender not connected. Unable to send beat message.");
        }
    }

    void OSCComponent::connectOSCSender(const String &address) {
        auto target = address + ":" + OSCPortString;
        auto success = sender.connect(address, OSCPort);
        if (!success) {
            logger.error("Error connecting OSC to " + target);
            return;
        }
        setSenderConnectedState(true);
        logger.info("Connected OSC to " + target);
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