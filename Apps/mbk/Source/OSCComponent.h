//
// Created by glyn on 11/12/2021.
//

#ifndef JUCECMAKEREPO_OSCCOMPONENT_H
#define JUCECMAKEREPO_OSCCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_osc/juce_osc.h>
#include "Logger.h"
#include "OSCSender.h"

namespace AudioApp {
class OSCComponent : public juce::Component, public OSCSender {
    public:
        explicit OSCComponent(Logger &logger);

        void paint(juce::Graphics &) override;

        void resized() override;

        bool send(const juce::OSCMessage &message);

    private:
        void connectOSCSender(const juce::String &);

        void disconnectOSCSender();

        void setSenderConnectedState(bool connected);

        Logger &logger;

        juce::Label targetAddress{"targetAddress", "127.0.0.1"};
        juce::TextButton connectOSCButton{"Connect OSC"};
        juce::OSCSender sender;
        bool senderConnected = false;
    };
}

#endif //JUCECMAKEREPO_OSCCOMPONENT_H
