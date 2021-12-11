#include "MainComponent.h"

namespace AudioApp
{
    MainComponent::MainComponent(): connectOSCButton("Connect OSC")
    {
        setAudioChannels(2,2);

        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);

        addAndMakeVisible(logger);
        addAndMakeVisible(audioSource);

        connectOSCButton.onClick = [this] { connectOSCSender(); };
        addAndMakeVisible(&connectOSCButton);

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
        audioSource.setBounds(getLocalBounds().removeFromTop(420));
        logger.setBounds(getLocalBounds()
            .withTrimmedBottom(50)
            .withTrimmedTop(420));
        tempoAnalyser.setBounds(getLocalBounds().removeFromBottom(50));
        connectOSCButton.setBounds(10, getHeight() - 90, getWidth() - 20, 30);
    }

    void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
    {
        audioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
        tempoAnalyser.updateSamplePerBlockExpected(samplesPerBlockExpected);
    }

    void MainComponent::releaseResources()
    {
    }

    void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        audioSource.getNextAudioBlock(bufferToFill);
        tempoAnalyser.processAudioFrame(audioSource.getFrameValues());
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

    void MainComponent::connectOSCSender()
    {
        senderConnected = sender.connect ("127.0.0.1", 9000);
        if (!senderConnected) {
            logger.log("Error: could not connect to UDP port 9001.");
            return;
        }
        logger.log("Connected OSC sender.");
    }
}