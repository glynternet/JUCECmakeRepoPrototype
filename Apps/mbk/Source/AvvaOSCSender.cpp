#include "AvvaOSCSender.h"

namespace AudioApp
{
    AvvaOSCSender::AvvaOSCSender(AudioApp::OSCSender& sender)
        : _sender(sender)
    {
    }

    bool AvvaOSCSender::sendLoudness(float loudness)
    {
        auto message = loudnessTemplate;
        message.addFloat32(loudness);
        return _sender.send(message);
    }

    bool AvvaOSCSender::sendFilePlaying()
    {
        auto message = filePlayingTemplateMessage;
        return _sender.send(message);
    }

    bool AvvaOSCSender::sendFilePaused()
    {
        auto message = filePausedTemplateMessage;
        return _sender.send(message);
    }

    bool AvvaOSCSender::sendFileStopped()
    {
        auto message = fileStoppedTemplateMessage;
        return _sender.send(message);
    }

    bool AvvaOSCSender::sendClockMillisPerBeat(float millisPerBeat)
    {
        auto message = clockMillisPerBeatTemplate;
        message.addFloat32(millisPerBeat);
        return _sender.send(message);
    }
}