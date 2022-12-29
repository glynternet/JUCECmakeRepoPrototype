#ifndef JUCECMAKEREPO_AvvaOSCSender_H
#define JUCECMAKEREPO_AvvaOSCSender_H

#include "JuceHeader.h"
#include "OSCSender.h"

namespace AudioApp
{
class AvvaOSCSender
{
private:
    AudioApp::OSCSender& _sender;

    juce::OSCMessage loudnessTemplate {"/audio", String("loudness"), (int32) 0};
    juce::OSCMessage filePlayingTemplateMessage {"/audio", String("filePlaying")};
    juce::OSCMessage filePausedTemplateMessage {"/audio", String("filePaused")};
    juce::OSCMessage fileStoppedTemplateMessage {"/audio", String("fileStopped")};
    juce::OSCMessage clockMillisPerBeatTemplate {"/clock",
                                                 (juce::String) "millisPerBeat"};

public:
    explicit AvvaOSCSender::AvvaOSCSender(AudioApp::OSCSender& sender);

    // sendLoudness will send a loudness OSC message using the configured client.
    bool sendLoudness(float loudness);

    // sendFilePlaying will send an OSC message using the configured client.
    bool sendFilePlaying();

    // sendFilePaused will send an OSC message using the configured client.
    bool sendFilePaused();

    // sendFileStopped will send an OSC message using the configured client.
    bool sendFileStopped();

    // sendLoudness will send a millisPerBeat OSC message using the configured client.
    bool sendClockMillisPerBeat(float millisPerBeat);
};
} // namespace AudioApp

#endif JUCECMAKEREPO_AvvaOSCSender_H
