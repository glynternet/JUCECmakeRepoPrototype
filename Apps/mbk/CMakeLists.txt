project(mbk VERSION 0.0.1)

set (TargetName ${PROJECT_NAME})

juce_add_gui_app(${TargetName} PRODUCT_NAME "mbk")
juce_generate_juce_header(${TargetName})

# I think this only needs to be the .cpp files
target_sources(${TargetName} PRIVATE
        Source/AvvaOSCSender.cpp
        Source/AudioSourceComponent.cpp
        Source/Components/LabelledSlider.cpp
        Source/Components/FlashBox.cpp
        Source/Components/LogOutputComponent.cpp
        Source/Loudness/Analyser.cpp
        Source/Loudness/TailOff.cpp
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainWindow.cpp
        Source/Logger/MultiLogger.cpp
        Source/OSCComponent.cpp
        Source/Logger/StdoutLogger.cpp
        Source/Beat/AnalyserComponent.cpp
        Source/Beat/SynthesizerComponent.cpp
        Libs/BTrack/BTrack.cpp
        Libs/BTrack/OnsetDetectionFunction.cpp)

# These are directories where headers are found, "something.h".
# Here we have to include the fftw and libsamplerate (which fftw depends on)
target_include_directories(${TargetName} PRIVATE
        ../../../dependencies/libsamplerate-0.2.2-win32
        ../../../dependencies/fftw3.3.5-dll32
        )

# Windows: These are directories that include .lib files.
target_link_directories(${TargetName} PRIVATE
        ../../../dependencies/libsamplerate-0.2.2-win32
        ../../../dependencies/fftw3.3.5-dll32
        )

target_compile_options(${TargetName} PRIVATE
        -DUSE_FFTW)

target_compile_definitions(${TargetName} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="mbk"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>")

target_link_libraries(${TargetName} PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        juce::juce_gui_basics
        juce::juce_osc
        juce::juce_dsp
        shared_processing_code
        # Libraries extra to the ones from the JUCE CMake template ones above.
        # Windows: These are the required `.lib` files but `.lib` is omitted from the definitions below.
        libfftw3-3
        samplerate
        )
