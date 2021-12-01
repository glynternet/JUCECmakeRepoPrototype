#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <shared_processing_code/shared_processing_code.h>
#include <juce_osc/juce_osc.h>

namespace AudioApp
{
//To save some typing, we're gonna import a few commonly used juce classes
//into our namespace
using juce::Colour;
using juce::Component;
using juce::Graphics;
using juce::String;
using juce::OSCSender;
} // namespace AudioApp