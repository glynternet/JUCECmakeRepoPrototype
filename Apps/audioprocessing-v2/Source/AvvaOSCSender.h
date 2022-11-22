#include "JuceHeader.h"

class AvvaOSCSender {
   private:
    juce::OSCSender sender;
    String _targetHostName;
    int _targetPortNumber = 9000;
    bool _connected = false;
    juce::OSCMessage loudnessTemplate;
    juce::OSCMessage filePlayingTemplateMessage;
    juce::OSCMessage filePausedTemplateMessage;
    juce::OSCMessage fileStoppedTemplateMessage;

   public:
    AvvaOSCSender(): loudnessTemplate("/audio", String("loudness"), (int32)0),
        filePlayingTemplateMessage("/audio", String("filePlaying")),
        filePausedTemplateMessage("/audio", String("filePaused")),
        fileStoppedTemplateMessage("/audio", String("fileStopped")) {
    }
    ~AvvaOSCSender() {}

    // connect prepares the sender for sending messages by binding to a local socket.
    // connect must be called before any messages are sent
    bool connect(const String targetHostName, const int targetPortNumber) {
        _targetHostName = targetHostName;
        _targetPortNumber = targetPortNumber;
        _connected = sender.connect(_targetHostName, _targetPortNumber);
        return _connected;
    }

    bool isConnected() { return _connected; }

    // sendLoudness will send a loudness OSC message to the configured host.
    // sendLoudness will not check that the sender is connected, for performance reasons.
    bool sendLoudness(float loudness) {
        auto message = loudnessTemplate;
        message.addFloat32(loudness);
        return sender.send(message);
    }

    // sendFilePlaying will send a OSC message to the configured host.
    // sendFilePlaying will not check that the sender is connected, for performance reasons.
    bool sendFilePlaying() {
        auto message = filePlayingTemplateMessage;
        return sender.send(message);
    }

    // sendFilePaused will send a OSC message to the configured host.
    // sendFilePaused will not check that the sender is connected, for performance reasons.
    bool sendFilePaused() {
        auto message = filePausedTemplateMessage;
        return sender.send(message);
    }

    // sendFileStopped will send a OSC message to the configured host.
    // sendFileStopped will not check that the sender is connected, for performance reasons.
    bool sendFileStopped() {
        auto message = fileStoppedTemplateMessage;
        return sender.send(message);
    }
};
