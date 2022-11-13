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

        Logger& logger;

        juce::Label targetAddress;
        juce::TextButton connectOSCButton;
        // Probably worth taking a look at the AVVAOSCSender class from the legacy repo
        juce::OSCSender sender;
        bool senderConnected = false;
    };
}

#endif //JUCECMAKEREPO_OSCCOMPONENT_H
