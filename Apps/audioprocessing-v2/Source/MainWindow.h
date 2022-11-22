#pragma once

#include "AnalyserComponent.h"

namespace AudioApp
{
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(const String& name);

private:
    void closeButtonPressed() override;
    Colour getBackgroundColour();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
}
