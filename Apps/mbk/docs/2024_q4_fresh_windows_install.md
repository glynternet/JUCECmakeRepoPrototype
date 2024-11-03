Fresh install of legacy CLion

Install legacy version of CLion, standard install from JetBrains site.

01 opening project in 
02 IDE loading CMake project, takes a while
03 Add a Visual Studio build system
04 Click Download to download visual studio and downloda the community version
05 Install only the _Desktop development with C++_ tools.
06 Building from that will likely hit some fftw3.h compile issue
FAILED: Apps/mbk/CMakeFiles/mbk.dir/Source/Beat/SynthesizerComponent.cpp.obj 
C:\PROGRA~1\JETBRA~1\CLION2~1.3\bin\mingw\bin\G__~1.EXE -DDEBUG=1 -DJUCE_APPLICATION_NAME_STRING=\"mbk\" -DJUCE_APPLICATION_VERSION_STRING=\"0.0.1\" -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_audio_devices=1 -DJUCE_MODULE_AVAILABLE_juce_audio_formats=1 -DJUCE_MODULE_AVAILABLE_juce_audio_processors=1 -DJUCE_MODULE_AVAILABLE_juce_audio_utils=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_data_structures=1 -DJUCE_MODULE_AVAILABLE_juce_dsp=1 -DJUCE_MODULE_AVAILABLE_juce_events=1 -DJUCE_MODULE_AVAILABLE_juce_graphics=1 -DJUCE_MODULE_AVAILABLE_juce_gui_basics=1 -DJUCE_MODULE_AVAILABLE_juce_gui_extra=1 -DJUCE_MODULE_AVAILABLE_juce_osc=1 -DJUCE_MODULE_AVAILABLE_shared_processing_code=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -D_DEBUG=1 -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/Apps/mbk/mbk_artefacts/JuceLibraryCode -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/../../../dependencies/libsamplerate-0.2.2-win32 -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/../../../dependencies/fftw3.3.5-dll32 -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/Modules -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/VST3_SDK -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/LV2_SDK -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/LV2_SDK/lv2 -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/LV2_SDK/serd -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/LV2_SDK/sord -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/LV2_SDK/sord/src -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/LV2_SDK/sratom -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/LV2_SDK/lilv -IC:/Users/g/avva/projects/JUCECmakeRepoPrototype/cmake-build-debug/_deps/juce-src/modules/juce_audio_processors/format_types/LV2_SDK/lilv/src -g -fdiagnostics-color=always -DUSE_FFTW -g -O0 -Wall -Wextra -Wpedantic -Wstrict-aliasing -Wuninitialized -Wunused-parameter -Wsign-compare -Wsign-conversion -Wunreachable-code -Wcast-align -Wno-implicit-fallthrough -Wno-maybe-uninitialized -Wno-ignored-qualifiers -Wswitch-enum -Wredundant-decls -Wno-strict-overflow -Wshadow -Woverloaded-virtual -Wreorder -Wzero-as-null-pointer-constant -Wa,-mbig-obj -MD -MT Apps/mbk/CMakeFiles/mbk.dir/Source/Beat/SynthesizerComponent.cpp.obj -MF Apps\mbk\CMakeFiles\mbk.dir\Source\Beat\SynthesizerComponent.cpp.obj.d -o Apps/mbk/CMakeFiles/mbk.dir/Source/Beat/SynthesizerComponent.cpp.obj -c C:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/Source/Beat/SynthesizerComponent.cpp
In file included from C:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/Libs/BTrack/BTrack.h:27,
                 from C:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/Source/Beat/SynthesizerComponent.h:4,
                 from C:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/Source/Beat/SynthesizerComponent.cpp:1:
C:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/Libs/BTrack/OnsetDetectionFunction.h:26:10: fatal error: fftw3.h: No such file or directory
   26 | #include "fftw3.h"
      |          ^~~~~~~~~

There is a file about getting BTrack to work and that contains the things attempted to fix the above fftw3 issue

Download fft 32 ad 64 bit v3.3.10 from https://www.fftw.org/download.html

Created `dependencies` dir next to the repo dir JUCECmakeRepoPrototype

```
dependencies/
└── fftw3.3.10-dll32
    └── fftw3.h
```

Downloaded 0.2.2 win32 and win64 from https://github.com/libsndfile/libsamplerate/releases/tag/0.2.2

The files for compile are all laid out in different places across the download.
Might be worth changing the cmake config in our repo so that the download can be used easier without moving files around.

Currently just the following, expecting compilation failure later
```
dependencies/
├── fftw3.3.10-dll32
│   └── fftw3.h
└── libsamplerate-0.2.2-win32
    └── samplerate.h
```

Failed because libs can't be found yet
```
FAILED: Apps/mbk/mbk_artefacts/Debug/mbk.exe 
cmd.exe /C "cd . && C:\PROGRA~1\JETBRA~1\CLION2~1.3\bin\mingw\bin\G__~1.EXE -g -mwindows Apps/mbk/CMakeFiles/mbk.dir/mbk_artefacts/JuceLibraryCode/resources.rc.obj Apps/mbk/CMakeFiles/mbk.dir/Source/AvvaOSCSender.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/AudioSourceComponent.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Components/LabelledSlider.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Components/FlashBox.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Components/LogOutputComponent.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Loudness/Analyser.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Loudness/TailOff.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Main.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/MainComponent.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/MainWindow.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Logger/MultiLogger.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/OSCComponent.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Logger/StdoutLogger.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Beat/AnalyserComponent.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Source/Beat/SynthesizerComponent.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Libs/BTrack/BTrack.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/Libs/BTrack/OnsetDetectionFunction.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_gui_basics/juce_gui_basics.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_graphics/juce_graphics.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_events/juce_events.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_core/juce_core.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_data_structures/juce_data_structures.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_osc/juce_osc.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_dsp/juce_dsp.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_audio_formats/juce_audio_formats.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_audio_basics/juce_audio_basics.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/Modules/shared_processing_code/shared_processing_code.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_audio_utils/juce_audio_utils.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_audio_processors/juce_audio_processors.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_audio_processors/juce_audio_processors_ara.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_audio_processors/juce_audio_processors_lv2_libs.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_gui_extra/juce_gui_extra.cpp.obj Apps/mbk/CMakeFiles/mbk.dir/__/__/_deps/juce-src/modules/juce_audio_devices/juce_audio_devices.cpp.obj -o Apps\mbk\mbk_artefacts\Debug\mbk.exe -Wl,--out-implib,Apps\mbk\mbk_artefacts\Debug\libmbk.dll.a -Wl,--major-image-version,0,--minor-image-version,0 -LC:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/../../../dependencies/libsamplerate-0.2.2-win32   -LC:/Users/g/avva/projects/JUCECmakeRepoPrototype/Apps/mbk/../../../dependencies/fftw3.3.10-dll32 -llibfftw3-3  -lsamplerate  -ldxgi  -luuid  -lwsock32  -lwininet  -lversion  -lole32  -lws2_32  -loleaut32  -limm32  -lcomdlg32  -lshlwapi  -lrpcrt4  -lwinmm  -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 && cd ."
C:\Program Files\JetBrains\CLion 2022.3.3\bin\mingw\bin/ld.exe: cannot find -llibfftw3-3
C:\Program Files\JetBrains\CLion 2022.3.3\bin\mingw\bin/ld.exe: cannot find -lsamplerate
collect2.exe: error: ld returned 1 exit status
ninja: build stopped: subcommand failed.
```

Moves libsamplerate.lib into deps
```
dependencies/
├── fftw3.3.10-dll32
│   └── fftw3.h
└── libsamplerate-0.2.2-win32
    ├── samplerate.h
    └── samplerate.lib
```

Now seeing some incompatible errors, could this be a 32 vs 64 bit issue?
```
dependencies/
├── fftw3.3.10-dll32
│   └── fftw3.h
└── libsamplerate-0.2.2-win32
    ├── samplerate.h
    └── samplerate.lib
```

Attempting to use the 64 bit version instead...
That seemed to work and now we just get the same issue for fftw3

```
C:\Program Files\JetBrains\CLion 2022.3.3\bin\mingw\bin/ld.exe: cannot find -llibfftw3-3
```

Opened _Developer Powershell for VS 2022_...
```
PS C:\Program Files\Microsoft Visual Studio\2022\Community> cd 'C:\Users\g\Downloads\fftw-3.3.5-dll64\'
PS C:\Users\g\Downloads\fftw-3.3.5-dll64> lib /machine:x64 /def:libfftw3-3.def
Microsoft (R) Library Manager Version 14.41.34123.0
Copyright (C) Microsoft Corporation.  All rights reserved.

   Creating library libfftw3-3.lib and object libfftw3-3.exp
PS C:\Users\g\Downloads\fftw-3.3.5-dll64> ls

    Directory: C:\Users\g\Downloads\fftw-3.3.5-dll64

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
...
-a----         11/3/2024   2:04 PM         241990 libfftw3-3.lib
...
```

Move file and make sure named as 64
```
dependencies/
├── fftw3.3.10-dll64
│   ├── fftw3.h
│   └── libfftw3-3.lib
└── libsamplerate-0.2.2-win64
    ├── samplerate.h
    └── samplerate.lib
```

---

TODO

- GETTING A PROD BUILD, NOT JUST DEBUG
- 64 bit?