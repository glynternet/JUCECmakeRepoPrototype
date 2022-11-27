#include "JuceHeader.h"
#include "../../mbk/Source/OSCSender.h"

class AvvaOSCSender
{
private:
    AudioApp::OSCSender& _sender;

    juce::OSCMessage loudnessTemplate {"/audio", String("loudness"), (int32) 0};
    juce::OSCMessage filePlayingTemplateMessage {"/audio", String("filePlaying")};
    juce::OSCMessage filePausedTemplateMessage {"/audio", String("filePaused")};
    juce::OSCMessage fileStoppedTemplateMessage {"/audio", String("fileStopped")};

public:
    explicit AvvaOSCSender::AvvaOSCSender(AudioApp::OSCSender& sender)
        : _sender(sender)
    {
    }

    ~AvvaOSCSender() = default;

    // sendLoudness will send a loudness OSC message to the configured host.
    bool sendLoudness(float loudness)
    {
        auto message = loudnessTemplate;
        message.addFloat32(loudness);
        return _sender.send(message);
    }

    // sendFilePlaying will send an OSC message to the configured host.
    bool sendFilePlaying()
    {
        auto message = filePlayingTemplateMessage;
        return _sender.send(message);
    }

    // sendFilePaused will send an OSC message to the configured host.
    bool sendFilePaused()
    {
        auto message = filePausedTemplateMessage;
        return _sender.send(message);
    }

    // sendFileStopped will send an OSC message to the configured host.
    bool sendFileStopped()
    {
        auto message = fileStoppedTemplateMessage;
        return _sender.send(message);
    }
};
