#include "JuceHeader.h"
#include "../../mbk/Source/OSCSender.h"

class AvvaOSCSender
{
private:
    AudioApp::OSCSender& _sender;
    juce::OSCMessage loudnessTemplate;
    juce::OSCMessage filePlayingTemplateMessage;
    juce::OSCMessage filePausedTemplateMessage;
    juce::OSCMessage fileStoppedTemplateMessage;

public:
    explicit AvvaOSCSender::AvvaOSCSender(AudioApp::OSCSender& sender)
        // TODO(glynternet): initialise these
        : loudnessTemplate("/audio", String("loudness"), (int32) 0)
        , filePlayingTemplateMessage("/audio", String("filePlaying"))
        , filePausedTemplateMessage("/audio", String("filePaused"))
        , fileStoppedTemplateMessage("/audio", String("fileStopped"))
        , _sender(sender) {}

    ~AvvaOSCSender() {}

    // sendLoudness will send a loudness OSC message to the configured host.
    // sendLoudness will not check that the sender is connected, for performance reasons.
    bool sendLoudness(float loudness)
    {
        auto message = loudnessTemplate;
        message.addFloat32(loudness);
        return _sender.send(message);
    }

    // sendFilePlaying will send a OSC message to the configured host.
    // sendFilePlaying will not check that the sender is connected, for performance reasons.
    bool sendFilePlaying()
    {
        auto message = filePlayingTemplateMessage;
        return _sender.send(message);
    }

    // sendFilePaused will send a OSC message to the configured host.
    // sendFilePaused will not check that the sender is connected, for performance reasons.
    bool sendFilePaused()
    {
        auto message = filePausedTemplateMessage;
        return _sender.send(message);
    }

    // sendFileStopped will send a OSC message to the configured host.
    // sendFileStopped will not check that the sender is connected, for performance reasons.
    bool sendFileStopped()
    {
        auto message = fileStoppedTemplateMessage;
        return _sender.send(message);
    }
};
