#include "MainComponent.h"

namespace AudioApp
{
    MainComponent::MainComponent(): state(Stopped), openButton("Open"), playButton("Play"), stopButton("Stop"), connectOSCButton("Connect OSC")
    {
        setAudioChannels(2,2);

        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);

        addAndMakeVisible(logger);

        openButton.onClick = [this] {  openButtonClicked(); };
        addAndMakeVisible(&openButton);

        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setEnabled(true);
        addAndMakeVisible(&playButton);

        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setEnabled(false);
        addAndMakeVisible(&stopButton);

        connectOSCButton.onClick = [this] { connectOSCSender(); };
        addAndMakeVisible(&connectOSCButton);

        formatManager.registerBasicFormats();
        transport.addChangeListener(this);

        addAndMakeVisible(selector);

        tempoAnalyser.onBeat = [this] { sendBeatMessage(); };
        addAndMakeVisible(tempoAnalyser);

        setSize (400, 700);
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
        logger.log("resized: " + getLocalBounds().toString());
        selector.setBounds(getLocalBounds().withTrimmedBottom(50));
        logger.setBounds(getLocalBounds()
            .withTrimmedBottom(50)
            .withTrimmedTop(420));
        tempoAnalyser.setBounds(getLocalBounds().removeFromBottom(50));
        int buttonWidth = (getWidth() - 40) / 3;
        openButton.setBounds(10, 310, buttonWidth, 30);
        playButton.setBounds(20 + buttonWidth, 310, buttonWidth, 30);
        stopButton.setBounds(30 + 2 * buttonWidth, 310, buttonWidth, 30);
        connectOSCButton.setBounds(10, 350, getWidth() - 20, 30);
    }

    void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
    {
        transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
        tempoAnalyser.updateSamplePerBlockExpected(samplesPerBlockExpected);
    }

    void MainComponent::releaseResources()
    {
    }

    void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        // TODO(glynternet): some logic around handling file vs device input.
        transport.getNextAudioBlock(bufferToFill);

        if (bufferToFill.buffer->getNumChannels() == 0) {
            logger.log("No channels in buffer to fill");
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        const auto* inputData = bufferToFill.buffer->getReadPointer (0, bufferToFill.startSample);
        auto* outputData = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);

        // as prescribed in BTrack README: https://github.com/adamstark/BTrack
        // TODO(glynternet): is there a float version of BTrack so that we can avoid this conversion of float to double and avoid creating a vector?
        // TODO(glynternet): if the above todo is not possible, can we reuse this vector to avoid having to create a new one every audio block?
        std::vector<double> frameValues(bufferToFill.numSamples);

        for (auto i = 0; i < bufferToFill.numSamples; ++i) {
            frameValues[i] = inputData[i];

            // TODO(glynternet): some logic around handling file vs device input.
            //   If from file, the transport will have already written to the output portion of the buffer.
            outputData[i] = inputData[i];
        }

        tempoAnalyser.processAudioFrame(frameValues.data());
    }

    void MainComponent::sendBeatMessage() {
        if (senderConnected) {
            try {
                logger.log("Message sent: " + std::to_string(sender.send("/hello")));
            }
            catch (const juce::OSCException& e) {
                logger.log("Error sending message: "+ e.description);
            }
        } else {
            logger.log("Sender not connected. Unable to send beat message.");
        }
    }

    void MainComponent::openButtonClicked()
    {
        logger.log("Open button clicked");
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

        logger.log("chooserClosed");

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

    void MainComponent::connectOSCSender()
    {
        senderConnected = sender.connect ("127.0.0.1", 9000);
        if (!senderConnected) {
            logger.log("Error: could not connect to UDP port 9001.");
            return;
        }
        logger.log("Connected OSC sender.");
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