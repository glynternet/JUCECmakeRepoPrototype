#include "MainComponent.h"

namespace AudioApp
{
MainComponent::MainComponent(): state(Stopped), openButton("Open"), playButton("Play"), stopButton("Stop")
{
    setAudioChannels(2,2);

    message.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    message.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(message);

    openButton.onClick = [this] {  openButtonClicked(); };
    addAndMakeVisible(&openButton);

    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(true);
    addAndMakeVisible(&playButton);

    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);
    addAndMakeVisible(&stopButton);

    formatManager.registerBasicFormats();
    transport.addChangeListener(this);

    setSize (400, 700);

    addAndMakeVisible(selector);

    tempoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
    tempoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(tempoLabel);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    message.setText("resized", juce::dontSendNotification);
    selector.setBounds(getLocalBounds().withTrimmedBottom(50));
    const juce::Rectangle<int> &tempoRectangle = getLocalBounds().removeFromBottom(50);
    message.setBounds(tempoRectangle.translated(0, -tempoRectangle.getHeight()));
    tempoLabel.setBounds(tempoRectangle);
    openButton.setBounds(10, 310, getWidth() - 20, 30);
    playButton.setBounds(10, 350, getWidth() - 20, 30);
    stopButton.setBounds(10, 390, getWidth() - 20, 30);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transport.prepareToPlay(samplesPerBlockExpected, sampleRate);

    btrackFrameSize = samplesPerBlockExpected;
    btrackHopSize = samplesPerBlockExpected / 2;
    b.updateHopAndFrameSize(btrackHopSize, btrackFrameSize);
    message.setText("Updated BTrack to with new hopSize:"+std::to_string(btrackHopSize)+" and frameSize:"+std::to_string(btrackFrameSize), juce::dontSendNotification);
}

void MainComponent::releaseResources()
{
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // TODO(glynternet): some logic around handling file vs device input.
    transport.getNextAudioBlock(bufferToFill);


    if (bufferToFill.numSamples != btrackFrameSize) {
        message.setText("Num samples not equal to frame size: frameSize:" + std::to_string(btrackFrameSize) + " numSamples:" + std::to_string(bufferToFill.numSamples), juce::dontSendNotification);
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (bufferToFill.buffer->getNumChannels() == 0) {
        message.setText("No channels in buffer to fill", juce::dontSendNotification);
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    const auto* channelData = bufferToFill.buffer->getReadPointer (0, bufferToFill.startSample);

    // as prescribed in BTrack README: https://github.com/adamstark/BTrack
    // TODO(glynternet): is there a float version of BTrack so that we can avoid this conversion of float to double and avoid creating a vector?
    // TODO(glynternet): if the above todo is not possible, can we reuse this vector to avoid having to create a new one every audio block?
    std::vector<double> frameValues(btrackFrameSize);

    for (auto i = 0; i < bufferToFill.numSamples; ++i) { frameValues[i] = channelData[i]; }

    b.processAudioFrame(frameValues.data());
    if (b.beatDueInCurrentFrame()) {
        tempoLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        juce::Timer::callAfterDelay(250, [this]{this->tempoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);});
        tempoLabel.setText(std::to_string(++beats) + " " + std::to_string( b.getCurrentTempoEstimate()), juce::dontSendNotification);
    }
}

void MainComponent::openButtonClicked()
    {
        message.setText("Open button clicked", juce::dontSendNotification);
        fileChooser_ = std::make_unique<juce::FileChooser> (("Choose a Patch to open..."),
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav; *.mp3");

        fileChooser_->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser &fileChooser) {
                this->chooserClosed(fileChooser);
            });
    }

    void MainComponent::chooserClosed(const juce::FileChooser& chooser){
        juce::File file (chooser.getResult());

        message.setText("chooserClosed", juce::dontSendNotification);
        //read the file
        juce::AudioFormatReader* reader = formatManager.createReaderFor(file);

        if (reader != nullptr)
        {
            //get the file ready to play
            std::unique_ptr<juce::AudioFormatReaderSource> tempSource (new juce::AudioFormatReaderSource (reader, true));

            transport.setSource(tempSource.get());
            transportStateChanged(Stopped);

            playSource.reset(tempSource.release());
        }
    }

    void MainComponent::playButtonClicked()
    {
        transportStateChanged(Starting);
    }

    void MainComponent::stopButtonClicked()
    {
        transportStateChanged(Stopping);
    }

    void MainComponent::transportStateChanged(TransportState newState)
    {
        if (newState != state)
        {
            state = newState;

            switch (state) {
                case Stopped:
                    playButton.setEnabled(true);
                    transport.setPosition(0.0);
                    break;

                case Playing:
                    playButton.setEnabled(true);
                    break;

                case Starting:
                    stopButton.setEnabled(true);
                    playButton.setEnabled(false);
                    transport.start();
                    break;

                case Stopping:
                    playButton.setEnabled(true);
                    stopButton.setEnabled(false);
                    transport.stop();
                    break;
            }
        }
    }

    void MainComponent::changeListenerCallback (juce::ChangeBroadcaster *source)
    {
        if (source == &transport)
        {
            if (transport.isPlaying())
            {
                transportStateChanged(Playing);
            }
            else
            {
                transportStateChanged(Stopped);
            }
        }
    }
}