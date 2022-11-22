#include "JuceHeader.h"
#include "FilePlayer.h"

class FilePlayerTransportComponent : public Component, private ChangeListener, private Timer {
   public:
    enum class TransportState { Uninitialised, Stopped, Starting, Playing, Pausing, Paused, Stopping };
    FilePlayerTransportComponent(int positionUpdateFrequency)
        : _positionUpdateFrequency(positionUpdateFrequency) {
        _filePlayer.addChangeListener(this);

        addAndMakeVisible(&positionLabel);
        positionLabel.setText(getTimerText(0), NotificationType::dontSendNotification);
        positionLabel.setJustificationType(Justification::centred);

        addAndMakeVisible(&openButton);
        openButton.setButtonText("Open...");
        openButton.onClick = [this]() { OnOpenClick(); };

        addAndMakeVisible(&playButton);
        playButton.setColour(TextButton::buttonColourId, Colours::green);
        playButton.onClick = [this]() { onPlayPauseClick(); };

        addAndMakeVisible(&stopButton);
        stopButton.setColour(TextButton::buttonColourId, Colours::red);
        stopButton.onClick = [this]() { _filePlayer.stop(); };

        setState(TransportState::Uninitialised);
    }

    ~FilePlayerTransportComponent() { _filePlayer.removeChangeListener(this); }

    void resized() override {
        auto width = getWidth();
        auto height = getHeight();
        auto buttonHeight = height / 4;
        positionLabel.setBounds(0, 0, width, buttonHeight);
        openButton.setBounds(0, buttonHeight, width, buttonHeight);
        playButton.setBounds(0, 2 * buttonHeight, width, buttonHeight);
        stopButton.setBounds(0, 3 * buttonHeight, width, buttonHeight);
    }

    void setState(TransportState state) {
        switch (state) {
            case TransportState::Uninitialised:
                playButton.setButtonText("Play");
                playButton.setEnabled(false);
                stopButton.setButtonText("Stop");
                stopButton.setEnabled(false);
                stopTimer();
                break;
            case TransportState::Stopped:
                playButton.setButtonText("Play");
                playButton.setEnabled(true);
                stopButton.setButtonText("Stop");
                stopButton.setEnabled(false);
                setLabelAsTime(&positionLabel, 0);
                stopTimer();
                break;
            case TransportState::Playing:
                playButton.setButtonText("Pause");
                playButton.setEnabled(true);
                stopButton.setButtonText("Stop");
                stopButton.setEnabled(true);
                startTimerHz(_positionUpdateFrequency);
                break;
            case TransportState::Paused:
                playButton.setButtonText("Resume");
                playButton.setEnabled(true);
                stopButton.setButtonText("Return to Zero");
                stopButton.setEnabled(true);
                stopTimer();
                break;
        }
    }

    void UpdatePositionLabel() {
        double pos = _filePlayer.getCurrentPosition();
        RelativeTime rt = RelativeTime::seconds(pos);
        setLabelAsTime(&positionLabel, rt.inMilliseconds());
    }


    std::function<void(FilePlayer::TransportState)> OnPlayPauseClick;


    void OnOpenClick() {
        fileChooser_ = std::make_unique<juce::FileChooser>(("Choose a supported file to play..."),
                                                           juce::File::getSpecialLocation(
                                                                   juce::File::userMusicDirectory),
                // TODO: MP3 doesn't seem to work on z30-a linux.
                //   Does it work on any other machine OS combo?
                                                           "*.wav;*.aiff;*.aif;*.flac");

        fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this](const juce::FileChooser &fileChooser) {
                                      this->chooserClosed(fileChooser);
                                  });
    };

    void chooserClosed(const juce::FileChooser &chooser) {
        juce::File file(chooser.getResult());
        setFile(file);
    }

    FilePlayer& getFilePlayer() { return _filePlayer; }

   private:

    std::unique_ptr<juce::FileChooser> fileChooser_;

    void logMessage(char const* message) { std::cout << message << "\n"; }

    bool setFile(File file) {
        bool success = _filePlayer.setFile(file);
        if (!success) {
            // TODO: put some good feedback here?
            return false;
        }
        setState(TransportState::Stopped);  // TODO: What if it isn't successful? Should it still be stopped?
        return true;
    }

    void onPlayPauseClick() {
        _filePlayer.playPause();
    }

    void timerCallback() override { UpdatePositionLabel(); }

    void changeListenerCallback(ChangeBroadcaster* source) override {
        if (source == &_filePlayer) {
            auto state = _filePlayer.getState();
            switch (state) {
                case FilePlayer::TransportState::Stopped:
                    setState(TransportState::Stopped);
                    logMessage("FilePlayerTransportComponent: state set to Stopped");
                    break;
                case FilePlayer::TransportState::Starting:
                    break;
                case FilePlayer::TransportState::Playing:
                    setState(TransportState::Playing);
                    logMessage("FilePlayerTransportComponent: state set to Playing");
                    break;
                case FilePlayer::TransportState::Pausing:
                    break;
                case FilePlayer::TransportState::Paused:
                    setState(TransportState::Paused);
                    logMessage("FilePlayerTransportComponent: state set to Paused");
                    break;
                case FilePlayer::TransportState::Stopping:
                    break;
            }
            if (this->OnPlayPauseClick != nullptr) {
                this->OnPlayPauseClick(state);
            }
            return;
        }
    }

    void setLabelAsTime(Label* label, int64 time) {
        std::string txt = getTimerText(time);
        label->setText(txt, dontSendNotification);
    }

    std::string getTimerText(int64 millis) {
        char buffer[12];  // 12 should be long enough to fit 0000:00:000 plus nul terminator
        int millisDisplay = millis % 1000;
        int secondsDisplay = (millis / 1000) % 60;
        int minutesDisplay = (millis / 60000) % 10000;
        sprintf(buffer, "%4d:%02d:%03d", minutesDisplay, secondsDisplay, millisDisplay);
        return std::string(buffer);
    }

    Label positionLabel;
    TextButton openButton;
    TextButton playButton;
    TextButton stopButton;

    int _positionUpdateFrequency;

    FilePlayer _filePlayer;
};