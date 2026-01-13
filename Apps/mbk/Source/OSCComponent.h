#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_osc/juce_osc.h>
#include "Logger/Logger.h"
#include "OSCSender.h"

namespace AudioApp {
class OSCComponent
    : public juce::Component
    , public OSCSender {
public:
    explicit OSCComponent(logger::Logger& logger);

    void paint(juce::Graphics&) override;

    void resized() override;

    bool send(const juce::OSCMessage& message) override;

private:
    void connectOSCSender(const juce::String&);

    void disconnectOSCSender();

    void setSenderConnectedState(bool connected);

    logger::Logger& logger;

    juce::uint32 lastNotConnectedLogMs = 0;
    static constexpr juce::uint32 notConnectedLogIntervalMs = 30000;

    juce::Label targetAddress {"targetAddress", "127.0.0.1"};
    juce::TextButton connectOSCButton {"Connect OSC"};
    juce::OSCSender sender;
    bool senderConnected = false;
};
} // namespace AudioApp
