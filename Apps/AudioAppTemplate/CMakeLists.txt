project(AudioAppTemplate VERSION 0.0.1)

set (TargetName ${PROJECT_NAME})

juce_add_gui_app(${TargetName} PRODUCT_NAME "Audio App Template")
juce_generate_juce_header(${TargetName})

# I think this only needs to include the .cpp files
target_sources(${TargetName} PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainWindow.cpp
        Source/BTrack/BTrack.cpp
#        Source/BTrack/BTrack.h
#        Libs/BTrack/CircularBuffer.h
        Source/BTrack/OnsetDetectionFunction.cpp
        Source/kissfft/kiss_fft.c
#        Libs/BTrack/OnsetDetectionFunction.h
        )

target_include_directories(${TargetName} PRIVATE
#        Libs/BTrack
#        Libs/kissfft
        ../../../../dependencies/fftw3.3.5-dll32
        ../../../../dependencies/libsamplerate-0.2.2-win32/include
        ../../../../dependencies/libsamplerate-0.2.2-win32
        )

target_compile_options(${TargetName} PRIVATE
        -DUSE_KISS_FFT)

target_compile_definitions(${TargetName} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_PROJECT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>")

target_link_directories(${TargetName} PRIVATE
        ../../../../dependencies/fftw3.3.5-dll32
        ../../../../dependencies/libsamplerate-0.2.2-win32/lib
        )

target_link_libraries(${TargetName} PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        juce::juce_gui_basics
        shared_processing_code
#        fftw3
        samplerate
        )
