#include "../JuceLibraryCode/JuceHeader.h"

class AudioInputSettingsComponent : public Component, public ChangeBroadcaster {
   public:
    AudioInputSettingsComponent() : monitorInputButton("Monitor Input") {
        addAndMakeVisible(audioInputMenu);
        audioInputMenu.addItem("From File", (int)AudioInput::fromFile);
        audioInputMenu.addItem("From Device", (int)AudioInput::fromDevice);
        audioInputMenu.setSelectedId((int)AudioInput::fromFile);
        audioInputMenu.onChange = [this]() { sendChangeMessage(); };

        addAndMakeVisible(monitorInputButton);
        monitorInputButton.onClick = [this]() { sendChangeMessage(); };
    }
    ~AudioInputSettingsComponent() {}

    enum class AudioInput { noneSelected, fromFile, fromDevice };

    void paint(Graphics&) override {}

    void resized() override {
        audioInputMenu.setBounds(0, 0, getWidth(), 20);
        monitorInputButton.setBounds(0, audioInputMenu.getHeight() + 3, getWidth(), 20);
    }

    AudioInput getSelectedInput() {
        int id = audioInputMenu.getSelectedId();
        switch (id) {
        case 0:
            return AudioInput::noneSelected;
        case 1:
            return AudioInput::fromFile;
        case 2:
            return AudioInput::fromDevice;
        default:
            throw new std::invalid_argument("Unsupported input value " + id);
        }
    }

    bool isMonitoringInput() { return monitorInputButton.getToggleState(); }

   private:
    ComboBox audioInputMenu;
    ToggleButton monitorInputButton;
};