#include "JuceHeader.h"

class FilePlayer : public AudioSource, public ChangeBroadcaster, public ChangeListener {
   public:
    enum class TransportState { Stopped, Starting, Playing, Pausing, Paused, Stopping };

    FilePlayer() {
        formatManager.registerBasicFormats();
        transportSource.addChangeListener(this);
    }

    ~FilePlayer() { transportSource.removeChangeListener(this); }

    void releaseResources() override { transportSource.releaseResources(); }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    // getNextAudioBlock fills the buffer with audio data from the class global
    // AudioTransportSource
    // If successful, getNextAudioBlockFromFile will return true, otherwise false
    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override {
        if (readerSource.get() == nullptr) {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        transportSource.getNextAudioBlock(bufferToFill);
    }

    void changeListenerCallback(ChangeBroadcaster* source) override {
        if (source == &transportSource) {
            if (transportSource.isPlaying())
                changeState(TransportState::Playing);
            else if ((TransportState::Stopping == state) || (TransportState::Playing == state))
                changeState(TransportState::Stopped);
            else if (TransportState::Pausing == state)
                changeState(TransportState::Paused);
            return;
        }
        sendChangeMessage();
    }

    void changeState(TransportState newState) {
        if (state == newState) {
            return;
        }

        auto oldState = state;
        state = newState;
        switch (state) {
            case TransportState::Stopped:
                transportSource.setPosition(0.0);
                logMessage("FilePlayer: transportSource position set to 0.0");
                break;

            case TransportState::Starting:
                transportSource.start();
                logMessage("FilePlayer: transportSource started");
                break;

            case TransportState::Playing:
                break;

            case TransportState::Pausing:
                transportSource.stop();
                logMessage("FilePlayer: transportSource stopped");
                break;

            case TransportState::Paused:
                break;

            case TransportState::Stopping:
                transportSource.stop();
                logMessage("FilePlayer: transportSource stopped");
                if (oldState == TransportState::Paused) {
                    // When going from Paused to Stopping, there will be no callback from the
                    // transport source to say that it has eventually become stopped.
                    // So here we need to manually set the state to stopped.
                    changeState(TransportState::Stopped);
                    logMessage("FilePlayer: oldState was paused, state changed to Stopped");
                }
                break;
        }

        sendChangeMessage();
    }

    TransportState getState() { return state; }

    bool isPlaying() { return transportSource.isPlaying(); }

    void playPause() {
        if ((state == TransportState::Stopped) || (state == TransportState::Paused)) {
            changeState(TransportState::Starting);
            return;
        }
        if (state == TransportState::Playing) {
            changeState(TransportState::Pausing);
        }
        // TODO: throw something?
    }

    void stop() { changeState(TransportState::Stopping); }

    bool setFile(File file) {
        auto* reader = formatManager.createReaderFor(file);
        if (reader == nullptr) {
            return false;
        }

        std::unique_ptr<AudioFormatReaderSource> newSource(
            new AudioFormatReaderSource(reader, true));
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource.reset(newSource.release());
        return true;
    }

    double getCurrentPosition() { return transportSource.getCurrentPosition(); }

    void setPosition(double pos) { transportSource.setPosition(pos); }

   private:
    void logMessage(char const* message) { std::cout << message << "\n"; }

    AudioTransportSource transportSource;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    AudioFormatManager formatManager;

    TransportState state = TransportState::Stopped;
};
