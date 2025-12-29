#include "MainWindow.h"

namespace AudioApp {
class GuiAppTemplateApplication : public juce::JUCEApplication {
public:
    // NOLINTNEXTLINE(readability-const-return-type) - JUCE override requires this signature
    const juce::String getApplicationName() override {
        return JUCE_APPLICATION_NAME_STRING;
    }
    // NOLINTNEXTLINE(readability-const-return-type) - JUCE override requires this signature
    const juce::String getApplicationVersion() override {
        return JUCE_APPLICATION_VERSION_STRING;
    }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& /*commandLine*/) override {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override { mainWindow.reset(); }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const juce::String& /*commandLine*/) override {}

private:
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace AudioApp

// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(AudioApp::GuiAppTemplateApplication)
