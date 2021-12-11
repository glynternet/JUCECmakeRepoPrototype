//
// Created by glyn on 11/12/2021.
//

#include "OSCComponent.h"

namespace AudioApp {
    OSCComponent::OSCComponent(Logger& logger): logger(logger), targetAddress("targetAddress", "localhost"), connectOSCButton("Connect OSC") {
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

    void OSCComponent::sendBeatMessage() {
        if (senderConnected) {
            try {
                logger.log("Message send " + (String)(sender.send("/hello") ? "success" : "error"));
            }
            catch (const juce::OSCException& e) {
                logger.log("Error sending message: "+ e.description);
            }
        } else {
            logger.log("Sender not connected. Unable to send beat message.");
        }
    }

    void OSCComponent::connectOSCSender(const String& address) {
        senderConnected = sender.connect (address, 9000);
        if (!senderConnected) {
            logger.log("Error: could not connect to UDP port 9001.");
            return;
        }
        logger.log("Connected OSC sender to "+address);
    }
}