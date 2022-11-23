project(audioprocessing-v2 VERSION 0.0.1)

set (TargetName ${PROJECT_NAME})

juce_add_gui_app(${TargetName} PRODUCT_NAME "audioprocessing-v2")
juce_generate_juce_header(${TargetName})

target_sources(${TargetName} PRIVATE
        Source/LabelledSlider.cpp
        Source/Main.cpp
        Source/MainWindow.cpp
        Source/TailOff.cpp
        Source/StdoutLogger.cpp
        ../mbk/Source/AudioSourceComponent.cpp
        )

target_compile_definitions(${TargetName} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="${PROJECT_NAME}"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>")

target_link_libraries(${TargetName} PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        juce::juce_gui_basics
        juce::juce_osc
        juce::juce_dsp
        shared_processing_code)