//
// Created by glyn on 11/12/2021.
//

#ifndef JUCECMAKEREPO_OSCCOMPONENT_H
#define JUCECMAKEREPO_OSCCOMPONENT_H
#include "CommonHeader.h"
#include "Logger.h"
namespace AudioApp {
    class OSCComponent : public juce::Component {
    public:
        explicit OSCComponent(Logger& logger);
        void paint(Graphics&) override;
        void resized() override;
        void sendBeatMessage(double duration);
    private:
        void connectOSCSender(const String&);
        void disconnectOSCSender();
        void setSenderConnectedState(bool connected);

        Logger& logger;

        juce::Label targetAddress {"targetAddress", "127.0.0.1" };
        juce::TextButton connectOSCButton { "Connect OSC" };
        // Probably worth taking a look at the AVVAOSCSender class from the legacy repo
        juce::OSCSender sender;
        bool senderConnected = false;
    };
}

#endif //JUCECMAKEREPO_OSCCOMPONENT_H
