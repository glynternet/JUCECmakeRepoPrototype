#include "MainComponent.h"

namespace AudioApp
{
    MainComponent::MainComponent(): state(Stopped), openButton("Open"), playButton("Play"), stopButton("Stop"), connectOSCButton("Connect OSC")
    {
        setAudioChannels(2,2);

        message.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        message.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(message);

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

        tempoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
        tempoLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(tempoLabel);
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
        log("resized");
        selector.setBounds(getLocalBounds().withTrimmedBottom(50));
        const juce::Rectangle<int> &tempoRectangle = getLocalBounds().removeFromBottom(50);
        message.setBounds(tempoRectangle.translated(0, -tempoRectangle.getHeight()));
        tempoLabel.setBounds(tempoRectangle);
        openButton.setBounds(10, 310, getWidth() - 20, 30);
        playButton.setBounds(10, 350, getWidth() - 20, 30);
        stopButton.setBounds(10, 390, getWidth() - 20, 30);
        connectOSCButton.setBounds(10, 430, getWidth() - 20, 30);
    }

    void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
    {
        transport.prepareToPlay(samplesPerBlockExpected, sampleRate);

        btrackFrameSize = samplesPerBlockExpected;
        btrackHopSize = samplesPerBlockExpected / 2;
        b.updateHopAndFrameSize(btrackHopSize, btrackFrameSize);
        log("Updated BTrack to with new hopSize:"+std::to_string(btrackHopSize)+" and frameSize:"+std::to_string(btrackFrameSize));
    }

    void MainComponent::releaseResources()
    {
    }

    static const int fadeIncrements = 8;

    void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        // TODO(glynternet): some logic around handling file vs device input.
        transport.getNextAudioBlock(bufferToFill);


        if (bufferToFill.numSamples != btrackFrameSize) {
            log("Num samples not equal to frame size: frameSize:" + std::to_string(btrackFrameSize) + " numSamples:" + std::to_string(bufferToFill.numSamples));
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        if (bufferToFill.buffer->getNumChannels() == 0) {
            log("No channels in buffer to fill");
            bufferToFill.clearActiveBufferRegion();
            return;
        }

    //    bufferToFill.buffer.mak
        const auto* inputData = bufferToFill.buffer->getReadPointer (0, bufferToFill.startSample);
        auto* outputData = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);



        // as prescribed in BTrack README: https://github.com/adamstark/BTrack
        // TODO(glynternet): is there a float version of BTrack so that we can avoid this conversion of float to double and avoid creating a vector?
        // TODO(glynternet): if the above todo is not possible, can we reuse this vector to avoid having to create a new one every audio block?
        std::vector<double> frameValues(btrackFrameSize);

        for (auto i = 0; i < bufferToFill.numSamples; ++i) {
            frameValues[i] = inputData[i];

            // TODO(glynternet): some logic around handling file vs device input.
            //   If from file, the transport will have already written to the output portion of the buffer.
            outputData[i] = inputData[i];
        }

        b.processAudioFrame(frameValues.data());
        if (b.beatDueInCurrentFrame()) {
            if (senderConnected) {
                try {
                    log("Message sent: " + std::to_string(sender.send("/hello")));
                }
                catch (const juce::OSCException& e) {
                    log("Error sending message: "+ e.description);
                }
            } else {
                log("Sender not connected. Unable to send beat message.");
            }

            tempoLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            double tempo = b.getCurrentTempoEstimate();

            auto current = juce::Time::currentTimeMillis();
            auto diff = current - lastTime;
            diffEwma = ewma(diffEwma, (double)diff, 0.1);
            lastTime = current;

            // this might actually be better as a function that says "repeat X time in the next Y milliseconds@
            for (int i = 0; i < fadeIncrements; ++i) {
                const double proportion = (float) i / float(fadeIncrements);
                // beatDueInCurrentFrame only happens every other beat and we want to fade over 2 beats, so we do
                // 1500 * seconds per beat to take 75% of the time between flashes to fade.
                // * 1000 to convert seconds to milliseconds
                // * 0.75 to convert to 75% of the time between "beats"
                // * 2 because the beat detection happens every other beat (there may be something in the research paper that mentions why this is)
                juce::Timer::callAfterDelay((int)((double)diffEwma * 0.75 * proportion), [this, proportion]{
                    this->tempoLabel.setColour (juce::Label::textColourId, juce::Colours::white.interpolatedWith(juce::Colours::lightgrey, (float)proportion));
                });
            }

            double tempoFromManualCalculation = 120000. / (double) diff;
            double tempoFromEWMA = 120000. / (double) diffEwma;
            ++beats;
            beat = ++beat%4;
            const int multiplier = 16;
            repeatFunc((int)(diffEwma/(double)multiplier), multiplier-1, [this, tempoFromManualCalculation, tempo, tempoFromEWMA]{
                beat = ++beat%4;
                tempoLabel.setText(
                        std::to_string(beats) + " " +
                        std::to_string(beat) + " " +
                        std::to_string(tempo) + " " +
                        std::to_string(diffEwma) + " " +
                        std::to_string(tempoFromManualCalculation) + " " +
                        std::to_string(tempoFromEWMA),
                        juce::dontSendNotification);
            });

            tempoLabel.setText(
                    std::to_string(beats) + " " +
                    std::to_string(beat) + " " +
                    std::to_string(tempo) + " " +
                    std::to_string(diffEwma) + " " +
                    std::to_string(tempoFromManualCalculation) + " " +
                    std::to_string(tempoFromEWMA),
                    juce::dontSendNotification);
        }
    }

    void MainComponent::repeatFunc(int interval, int count, std::function<void()> call) {
        // TODO: use a single timer here instead
        for (int i = 0; i < count; ++i) {
            juce::Timer::callAfterDelay((1+i)*interval, call);
        }
    }

    double MainComponent::ewma(double current, double nextValue, double alpha) const { return alpha * nextValue + (1 - alpha) * current; }

    void MainComponent::openButtonClicked()
    {
        log("Open button clicked");
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

        log("chooserClosed");

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
            log("Error: could not connect to UDP port 9001.");
            return;
        }
        log("Connected OSC sender.");
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

    void MainComponent::log(const String& message) {
        this->message.setText(message, juce::dontSendNotification);
    }
}