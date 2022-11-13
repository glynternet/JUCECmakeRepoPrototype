//
// Created by glyn on 11/12/2021.
//

#include "OSCComponent.h"
#include <memory>

namespace AudioApp {
    static const int OSCPort = 9000;
    static const std::string OSCPortString = std::to_string(OSCPort);

    // the value 123 is currently ignored on the server side
    static const std::unique_ptr<juce::OSCMessage> clockMessage = std::make_unique<juce::OSCMessage>("/clock", (juce::String)"millisPerBeat", (float)123);

    OSCComponent::OSCComponent(Logger& l): logger(l), targetAddress("targetAddress", "127.0.0.1"), connectOSCButton("Connect OSC") {
        targetAddress.setJustificationType(juce::Justification::centred);
        targetAddress.setEditable(true, false, true);
        targetAddress.onEditorShow = [this]{
            connectOSCButton.setEnabled(false);
        };
        targetAddress.onEditorHide = [this]{
            connectOSCButton.setEnabled(targetAddress.getText().length() > 0);
        };
        connectOSCButton.onClick = [this] { connectOSCSender(targetAddress.getText(true)); };
        addAndMakeVisible(connectOSCButton);
        addAndMakeVisible(targetAddress);
    }

    void OSCComponent::resized() {
        auto addressArea = getLocalBounds().expanded(-10);
        auto connectButtonBounds = addressArea.removeFromRight(addressArea.getWidth()/2);
        targetAddress.setBounds(addressArea);
        connectOSCButton.setBounds(connectButtonBounds);
    }

    void OSCComponent::paint(Graphics&) {}

    void OSCComponent::sendBeatMessage(double period) {
        if (senderConnected) {
            try {
                // modify period argument of message
                (*clockMessage)[1] = (float)period;
                if (sender.send(*clockMessage)) {
                    logger.debug("Message sent");
                } else {
                    logger.error("Error sending message");
                }
            }
            catch (const juce::OSCException& e) {
                logger.error("Error sending message: "+ e.description);
            }
        } else {
            logger.info("Sender not connected. Unable to send beat message.");
        }
    }

    void OSCComponent::connectOSCSender(const String& address) {
        auto target = address + ":" + OSCPortString;
        senderConnected = sender.connect (address, OSCPort);
        if (!senderConnected) {
            logger.error("Error: could not connect to UDP port " + OSCPortString);
            return;
        }
        logger.info("Connected OSC sender to " + address + ":" + OSCPortString);
    }
}