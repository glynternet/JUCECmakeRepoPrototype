project(AudioAppTemplate VERSION 0.0.1)

set (TargetName ${PROJECT_NAME})

juce_add_gui_app(${TargetName} PRODUCT_NAME "Audio App Template")
juce_generate_juce_header(${TargetName})

# I think this only needs to be the .cpp files
target_sources(${TargetName} PRIVATE
        Source/AudioSourceComponent.cpp
        Source/LogOutputComponent.cpp
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainWindow.cpp
        Source/OSCComponent.cpp
        Source/TempoAnalyserComponent.cpp
        Source/TempoSynthesizerComponent.cpp
        Libs/BTrack/BTrack.cpp
        Libs/BTrack/OnsetDetectionFunction.cpp)

# These are directories where headers are found, "something.h".
# Here we have to include the fftw and libsamplerate (which fftw depends on)
target_include_directories(${TargetName} PRIVATE
        ../../../../dependencies/libsamplerate-0.2.2-win32/include
        ../../../../dependencies/fftw3.3.5-dll32
        )

# Windows: These are directories that include .lib files.
target_link_directories(${TargetName} PRIVATE
        ../../../../dependencies/libsamplerate-0.2.2-win32/lib
        ../../../../dependencies/fftw3.3.5-dll32
        )

target_compile_options(${TargetName} PRIVATE
        -DUSE_FFTW)

target_compile_definitions(${TargetName} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_PROJECT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>")

target_link_libraries(${TargetName} PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        juce::juce_gui_basics
        juce::juce_osc
        shared_processing_code
        # Libraries extra to the ones from the JUCE CMake template ones above.
        # Windows: These are the required `.lib` files but `.lib` is omitted from the definitions below.
        #          The built artifact of fftw3 is actually libfftw3-3.lib and needs to be renamed to fftw3.lib
        fftw3
        samplerate
        )
