## Log

### Cannot find `fftw3.h`
```
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\OnsetDetectionFunction.h(26): fatal error C1083: Cannot open include file: 'fftw3.h': No such file or directory
```

This is because the BTrack is trying to use the fftw library and the header file cannoty be found.

Trying to get the FFTW library and fix this.

Fetching FFTW from http://www.fftw.org/install/windows.html

In there page they mention "lib", which is available as part of VS 2019 and found in the _Developer PowerShell for VS 2019_ (which has bash-esque commands like `ls`, `cd`).

When it comes to linter errors for BTrack, another option would be to try to add a build system to BTrack so that I don't have to deal with any code issues (syntax, linting, etc.) within that repo but that seems like quite a task for me right now.

```
PS C:\Users\glynh\avva\dependencies\fftw3.3.5-dll64> lib /machine:x64 /def:libfftw3l-3.def
>>                                                                  
Microsoft (R) Library Manager Version 14.27.29111.0
Copyright (C) Microsoft Corporation.  All rights reserved.                                                                                   
	Creating library libfftw3l-3.lib and object libfftw3l-3.exp
```

So now we have this `libfftw3l-3.lib` file.

This solved the issues of not being able to locate the header file and moved onto some warnings and errors in the code files. 

### Code linter warnings and errors

```
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Source\MainComponent.cpp(40): warning C4100: 'sampleRate': unreferenced formal parameter
```
`unreferenced formal parameter` just means that the parameter is unused fixed my removing parameter name.

```
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Source\MainComponent.cpp(69): error C2131: expression did not evaluate to a constant
```
This is because btrackFrameSize is not a constant and so this cannot be initialised as an array, need to use a vector instead.
```c++
// replace this
double frameValues[btrackFrameSize];

// with this
std::vector<double> frameValues(btrackFrameSize);
```

Once the above issues were solved, the following issues were dissolved.
```
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Source\MainComponent.cpp(69): note: failure was caused by a read of a variable outside its lifetime
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Source\MainComponent.cpp(69): note: see usage of 'this'
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Source\MainComponent.cpp(72): error C3863: array type 'double [this->2696]' is not assignable
```

### Cannot find `samplerate.h`

_note: in a later step of working this out I found that the version of libsamplerate I was using was not the latest and that libsamplerate publish a Windows artifact to use, so probably worth reading ahead before actioning anything based on this step._

```
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\BTrack.cpp(25): fatal error C1083: Cannot open include file: 'samplerate.h': No such file or directory
```

Getting libsamplerate from http://www.mega-nerd.com/SRC/libsamplerate-0.1.9.tar.gz

Tried putting in sample place as fftw3 and adding the source dir of libsamplerate
```
target_include_directories(${TargetName} PRIVATE
		...
        ../../../../dependencies/libsamplerate-0.1.9/src
```

This solved the "No such file"... issue and then revealed a load of linter issues with BTrack.

### BTrack linter issues

Realistically I don't want to touch BTrack because it _should_ work as it is and I don't want to risk breaking it.

```

C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\BTrack.cpp(89): warning C4100: 'frameSize_': unreferenced formal parameter
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\BTrack.cpp(275): warning C4458: declaration of 'tempo' hides class member
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\BTrack.h(207): note: see declaration of 'BTrack::tempo'
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\BTrack.cpp(342): warning C4458: declaration of 'tempo' hides class member
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\BTrack.h(207): note: see declaration of 'BTrack::tempo'
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\BTrack.cpp(383): error C2131: expression did not evaluate to a constant
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\Libs\BTrack\BTrack.cpp(383): note: failure was caused by a read of a variable outside its lifetime
...
```

So ideally I would just turn these errors off or something like that.

Looks like I can use `target_include_directories` instead of including the BTrack files as source and they won't get linter checked.

```diff
diff --git a/Apps/AudioAppTemplate/CMakeLists.txt b/Apps/AudioAppTemplate/CMakeLists.txt                             
index 48e0574..e5db6cb 100755                                                                                       
 --- a/Apps/AudioAppTemplate/CMakeLists.txt                                                                           
 +++ b/Apps/AudioAppTemplate/CMakeLists.txt                                                                           
@@ -9,15 +9,13 @@ target_sources(${TargetName} PRIVATE                                                                        
Source/Main.cpp                                                                                                      
Source/MainComponent.cpp                                                                                             
Source/MainWindow.cpp                                                                                       
-        Libs/BTrack/BTrack.cpp                                                                                      
-        Libs/BTrack/BTrack.h                                                                                        
-        Libs/BTrack/CircularBuffer.h                                                                                
-        Libs/BTrack/OnsetDetectionFunction.cpp                                                                      
-        Libs/BTrack/OnsetDetectionFunction.h                                                                                 
 )                                                                                                                                                                                                                                 
 target_include_directories(${TargetName} PRIVATE                                                                    
+        Libs/BTrack                                                                                                 
```

### Linker issues

Now it looks like the code can compile but can't be linked properly.
```
LINK Pass 1: command "C:\PROGRA~2\MICROS~2\2019\COMMUN~1\VC\Tools\MSVC\1427~1.291\bin\Hostx86\x86\link.exe /nologo @CMakeFiles\AudioAppTemplate.dir\objects1.rsp /out:AudioAppTemplate_artefacts\Debug\Audio App Template.exe /implib:AudioAppTemplate_artefacts\Debug\Audio App Template.lib /pdb:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\cmake-build-debug\Apps\AudioAppTemplate\AudioAppTemplate_artefacts\Debug\Audio App Template.pdb /version:0.0 /machine:X86 /debug /INCREMENTAL /subsystem:windows fftw3.lib samplerate.lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST /MANIFESTFILE:CMakeFiles\AudioAppTemplate.dir/intermediate.manifest CMakeFiles\AudioAppTemplate.dir/manifest.res" failed (exit code 1104) with the following output:
LINK : fatal error LNK1104: cannot open file 'fftw3.lib'
```

So we have to find a way for `fftw3.lib` to become available to the linker.

Trying to use `target_link_directories`, as so:
```cmake
target_link_directories(${TargetName} PRIVATE
		// this is where I build the .lib file in the notes a few steps above
        ../../../../dependencies/fftw3.3.5-dll64
        )
```

Getting the same errors but I am seeing that the added directory is being included in the `-LIBPATH`

```
LINK Pass 1: command "C:\PROGRA~2\MICROS~2\2019\COMMUN~1\VC\Tools\MSVC\1427~1.291\bin\Hostx86\x86\link.exe /nologo @CMakeFiles\AudioAppTemplate.dir\objects1.rsp /out:AudioAppTemplate_artefacts\Debug\Audio App Template.exe /implib:AudioAppTemplate_artefacts\Debug\Audio App Template.lib /pdb:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\cmake-build-debug\Apps\AudioAppTemplate\AudioAppTemplate_artefacts\Debug\Audio App Template.pdb /version:0.0 /machine:X86 /debug /INCREMENTAL /subsystem:windows -LIBPATH:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\..\..\..\..\dependencies\fftw3.3.5-dll64 fftw3.lib samplerate.lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST /MANIFESTFILE:CMakeFiles\AudioAppTemplate.dir/intermediate.manifest CMakeFiles\AudioAppTemplate.dir/manifest.res" failed (exit code 1104) with the following output:
LINK : fatal error LNK1104: cannot open file 'fftw3.lib'
```

Removing `fftw3` from the `target_link_libraries` config stopped this error and moved onto the same error but for samplerate.lib: `cannot open file 'samplerate.lib'`

Making samplerate.lib I seem to remember being in the docs, so checking there now...

Looks like to compile libsample rate for windows we need to get hold of CMake for windows.
* https://cmake.org/download/
* https://cmake.org/runningcmake/


Looks like libsample rate already actually create the .lib files for us in the releases. So redownloading from GitHub as a version built for Windows: https://github.com/libsndfile/libsamplerate/releases/tag/0.2.2

Changing original libsample configs to 
```
target_include_directories(${TargetName} PRIVATE
        ../../../../dependencies/libsamplerate-0.2.2-win64/include
```

```
target_link_directories(${TargetName} PRIVATE
        ../../../../dependencies/fftw3.3.5-dll64
        ../../../../dependencies/libsamplerate-0.2.2-win64/lib
```

Looks like the provided lib from libsamplerate is for x64 and the cmake build we are trying to make using x86
```
C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\..\..\..\..\dependencies\libsamplerate-0.2.2-win64\lib\samplerate.lib : warning LNK4272: library machine type 'x64' conflicts with target machine type 'x86'
AudioAppTemplate_artefacts\Debug\Audio App Template.exe : fatal error LNK1120: 6 unresolved externals
```

Downloaded the win32 one instead and trying using lib and source from that :joy:

That seems to have solved it, moving onto some "unresolved externals" issues.

### Library improvement (couldn't get to work)
While looking into the above error I found this stack overflow https://stackoverflow.com/questions/55551536/getting-unresolved-external-symbol-error-with-cmake-and-visual-studio
It suggests the following way of referencing an external library

```cmake
add_library(Allegro SHARED IMPORTED)
set_target_properties(Allegro PROPERTIES
    IMPORTED_LOCATION "${DEPENDENCIES_DIR}/allegro/bin/allegro.dll"
    IMPORTED_IMPLIB "${DEPENDENCIES_DIR}/allegro/lib/allegro.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${DEPENDENCIES_DIR}/allegro/include"
)
and then you just use it like

target_link_libraries(core … Allegro)
```

This gave me issues where it said, "don't know how to build PATH/TO/libramplerate.lib"

### BTrack linking  errors

Lots of `unresolved external symbol`

```
LINK Pass 1: command "C:\PROGRA~2\MICROS~2\2019\COMMUN~1\VC\Tools\MSVC\1427~1.291\bin\Hostx86\x86\link.exe /nologo @CMakeFiles\AudioAppTemplate.dir\objects1.rsp /out:AudioAppTemplate_artefacts\Debug\Audio App Template.exe /implib:AudioAppTemplate_artefacts\Debug\Audio App Template.lib /pdb:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\cmake-build-debug\Apps\AudioAppTemplate\AudioAppTemplate_artefacts\Debug\Audio App Template.pdb /version:0.0 /machine:X86 /debug /INCREMENTAL /subsystem:windows -LIBPATH:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\..\..\..\..\dependencies\fftw3.3.5-dll64 -LIBPATH:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\..\..\..\..\dependencies\libsamplerate-0.2.2-win32\lib samplerate.lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST /MANIFESTFILE:CMakeFiles\AudioAppTemplate.dir/intermediate.manifest CMakeFiles\AudioAppTemplate.dir/manifest.res" failed (exit code 1120) with the following output:
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: __thiscall BTrack::BTrack(int,int)" (??0BTrack@@QAE@HH@Z) referenced in function "public: __thiscall AudioApp::MainComponent::MainComponent(void)" (??0MainComponent@AudioApp@@QAE@XZ)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: __thiscall BTrack::~BTrack(void)" (??1BTrack@@QAE@XZ) referenced in function "public: virtual __thiscall AudioApp::MainComponent::~MainComponent(void)" (??1MainComponent@AudioApp@@UAE@XZ)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: void __thiscall BTrack::updateHopAndFrameSize(int,int)" (?updateHopAndFrameSize@BTrack@@QAEXHH@Z) referenced in function "public: virtual void __thiscall AudioApp::MainComponent::prepareToPlay(int,double)" (?prepareToPlay@MainComponent@AudioApp@@UAEXHN@Z)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: void __thiscall BTrack::processAudioFrame(double *)" (?processAudioFrame@BTrack@@QAEXPAN@Z) referenced in function "public: virtual void __thiscall AudioApp::MainComponent::getNextAudioBlock(struct juce::AudioSourceChannelInfo const &)" (?getNextAudioBlock@MainComponent@AudioApp@@UAEXABUAudioSourceChannelInfo@juce@@@Z)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: bool __thiscall BTrack::beatDueInCurrentFrame(void)" (?beatDueInCurrentFrame@BTrack@@QAE_NXZ) referenced in function "public: virtual void __thiscall AudioApp::MainComponent::getNextAudioBlock(struct juce::AudioSourceChannelInfo const &)" (?getNextAudioBlock@MainComponent@AudioApp@@UAEXABUAudioSourceChannelInfo@juce@@@Z)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: double __thiscall BTrack::getCurrentTempoEstimate(void)" (?getCurrentTempoEstimate@BTrack@@QAENXZ) referenced in function "public: virtual void __thiscall AudioApp::MainComponent::getNextAudioBlock(struct juce::AudioSourceChannelInfo const &)" (?getNextAudioBlock@MainComponent@AudioApp@@UAEXABUAudioSourceChannelInfo@juce@@@Z)
AudioAppTemplate_artefacts\Debug\Audio App Template.exe : fatal error LNK1120: 6 unresolved externals
```

Looks like the Btrack files just aren't available.
Maybe I can try remove the stage where I removed the reference to each file and replaced with the following. I am assuming this is going to give me linter errors thought :( 
```
target_include_directories(${TargetName} PRIVATE
        Libs/BTrack
```

This did give linter (maybe more extreme that linter) warning and errors. Attempting to fix just the errors to see if this solution above will work anyway.
Basically this was replacing use of arrays with non-constant initialisers to using vectors.
But this did not solve the `unresolved external symbol` issue :(.

Getting issues of the following, looking like btrack can't find the fft references
```
LINK Pass 1: command "C:\PROGRA~2\MICROS~2\2019\COMMUN~1\VC\Tools\MSVC\1427~1.291\bin\Hostx86\x86\link.exe /nologo @CMakeFiles\AudioAppTemplate.dir\objects1.rsp /out:AudioAppTemplate_artefacts\Debug\Audio App Template.exe /implib:AudioAppTemplate_artefacts\Debug\Audio App Template.lib /pdb:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\cmake-build-debug\Apps\AudioAppTemplate\AudioAppTemplate_artefacts\Debug\Audio App Template.pdb /version:0.0 /machine:X86 /debug /INCREMENTAL /subsystem:windows -LIBPATH:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\..\..\..\..\dependencies\fftw3.3.5-dll64 -LIBPATH:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\..\..\..\..\dependencies\libsamplerate-0.2.2-win32\lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST /MANIFESTFILE:CMakeFiles\AudioAppTemplate.dir/intermediate.manifest CMakeFiles\AudioAppTemplate.dir/manifest.res" failed (exit code 1120) with the following output:
BTrack.cpp.obj : error LNK2019: unresolved external symbol __imp__fftw_execute referenced in function "private: void __thiscall BTrack::calculateBalancedACF(double *)" (?calculateBalancedACF@BTrack@@AAEXPAN@Z)
OnsetDetectionFunction.cpp.obj : error LNK2001: unresolved external symbol __imp__fftw_execute
BTrack.cpp.obj : error LNK2019: unresolved external symbol __imp__fftw_plan_dft_1d referenced in function "private: void __thiscall BTrack::initialise(int,int)" (?initialise@BTrack@@AAEXHH@Z)
OnsetDetectionFunction.cpp.obj : error LNK2001: unresolved external symbol __imp__fftw_plan_dft_1d
BTrack.cpp.obj : error LNK2019: unresolved external symbol __imp__fftw_destroy_plan referenced in function "public: __thiscall BTrack::~BTrack(void)" (??1BTrack@@QAE@XZ)
OnsetDetectionFunction.cpp.obj : error LNK2001: unresolved external symbol __imp__fftw_destroy_plan
BTrack.cpp.obj : error LNK2019: unresolved external symbol __imp__fftw_malloc referenced in function "private: void __thiscall BTrack::initialise(int,int)" (?initialise@BTrack@@AAEXHH@Z)
OnsetDetectionFunction.cpp.obj : error LNK2001: unresolved external symbol __imp__fftw_malloc
BTrack.cpp.obj : error LNK2019: unresolved external symbol __imp__fftw_free referenced in function "public: __thiscall BTrack::~BTrack(void)" (??1BTrack@@QAE@XZ)
OnsetDetectionFunction.cpp.obj : error LNK2001: unresolved external symbol __imp__fftw_free
BTrack.cpp.obj : error LNK2019: unresolved external symbol _src_simple referenced in function "private: void __thiscall BTrack::resampleOnsetDetectionFunction(void)" (?resampleOnsetDetectionFunction@BTrack@@AAEXXZ)
AudioAppTemplate_artefacts\Debug\Audio App Template.exe : fatal error LNK1120: 6 unresolved externals
```

Moving BTrack back to being in `target_include_directories` takes us back to "6 unresolved externals", all in MainComponent and all based on BTrack

```
LINK Pass 1: command "C:\PROGRA~2\MICROS~2\2019\COMMUN~1\VC\Tools\MSVC\1427~1.291\bin\Hostx86\x86\link.exe /nologo @CMakeFiles\AudioAppTemplate.dir\objects1.rsp /out:AudioAppTemplate_artefacts\Debug\Audio App Template.exe /implib:AudioAppTemplate_artefacts\Debug\Audio App Template.lib /pdb:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\cmake-build-debug\Apps\AudioAppTemplate\AudioAppTemplate_artefacts\Debug\Audio App Template.pdb /version:0.0 /machine:X86 /debug /INCREMENTAL /subsystem:windows -LIBPATH:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\..\..\..\..\dependencies\fftw3.3.5-dll64 -LIBPATH:C:\Users\glynh\avva\projects\JUCECmakeRepoPrototype\Apps\AudioAppTemplate\..\..\..\..\dependencies\libsamplerate-0.2.2-win32\lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST /MANIFESTFILE:CMakeFiles\AudioAppTemplate.dir/intermediate.manifest CMakeFiles\AudioAppTemplate.dir/manifest.res" failed (exit code 1120) with the following output:
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: __thiscall BTrack::BTrack(int,int)" (??0BTrack@@QAE@HH@Z) referenced in function "public: __thiscall AudioApp::MainComponent::MainComponent(void)" (??0MainComponent@AudioApp@@QAE@XZ)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: __thiscall BTrack::~BTrack(void)" (??1BTrack@@QAE@XZ) referenced in function "public: virtual __thiscall AudioApp::MainComponent::~MainComponent(void)" (??1MainComponent@AudioApp@@UAE@XZ)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: void __thiscall BTrack::updateHopAndFrameSize(int,int)" (?updateHopAndFrameSize@BTrack@@QAEXHH@Z) referenced in function "public: virtual void __thiscall AudioApp::MainComponent::prepareToPlay(int,double)" (?prepareToPlay@MainComponent@AudioApp@@UAEXHN@Z)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: void __thiscall BTrack::processAudioFrame(double *)" (?processAudioFrame@BTrack@@QAEXPAN@Z) referenced in function "public: virtual void __thiscall AudioApp::MainComponent::getNextAudioBlock(struct juce::AudioSourceChannelInfo const &)" (?getNextAudioBlock@MainComponent@AudioApp@@UAEXABUAudioSourceChannelInfo@juce@@@Z)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: bool __thiscall BTrack::beatDueInCurrentFrame(void)" (?beatDueInCurrentFrame@BTrack@@QAE_NXZ) referenced in function "public: virtual void __thiscall AudioApp::MainComponent::getNextAudioBlock(struct juce::AudioSourceChannelInfo const &)" (?getNextAudioBlock@MainComponent@AudioApp@@UAEXABUAudioSourceChannelInfo@juce@@@Z)
MainComponent.cpp.obj : error LNK2019: unresolved external symbol "public: double __thiscall BTrack::getCurrentTempoEstimate(void)" (?getCurrentTempoEstimate@BTrack@@QAENXZ) referenced in function "public: virtual void __thiscall AudioApp::MainComponent::getNextAudioBlock(struct juce::AudioSourceChannelInfo const &)" (?getNextAudioBlock@MainComponent@AudioApp@@UAEXABUAudioSourceChannelInfo@juce@@@Z)
AudioAppTemplate_artefacts\Debug\Audio App Template.exe : fatal error LNK1120: 6 unresolved externals
```

How about using the 32 bit FFTW?

Same issues.

Going to try moving all BTrack code into the same folder as the MainComponent code :shrug:

Back to the thing where fftw can't be found for the BTrack source.

Trying to add back `fftw3` and `samplerate` to. the `target_link_libraries`

Now getting an issue with `LINK : fatal error LNK1104: cannot open file 'fftw3.lib'`


### Attempting again to use kiss ftw so that the source code can be hosted within the repo.

Cloning repo and removing all CMake stuff so it doesn't start getting compiled in CLion

```
git clone ssh://git@github.com/mborgerding/
kissfft
cd ./kissfft/
ls
rm -rf ./cmake ./CMakeLists.txt ./tools ./test  
```

### Got fftw to work

Saw that the error message was shoing fftw3.lib, checked for exact file in the `../../../../dependencies/fftw3.3.5-dll32` directory and saw that the lib file there was called `libfftw3l-3.lib`. Not only was fftw3 wrong, but I'd compile the wrong lib.
Recompiling the lib from the correct `.def` file...
```
PS C:\Users\glynh\avva\dependencies\fftw3.3.5-dll32> lib /machine:x86 /def:libfftw3-3.def
Microsoft (R) Library Manager Version 14.27.29111.0
Copyright (C) Microsoft Corporation.  All rights reserved.
Creating library libfftw3-3.lib and object libfftw3-3.exp  
```

Now we have `libfftw3-3.lib` created and can use `libfftw3-3` in the `target_link_libraries` call.

### Voila

Now it works and I can backtrack to remove any kruft that I didn't want to do.

## References

### CMake reference pages
[target_include_directories](https://cmake.org/cmake/help/latest/command/target_include_directories.html)
Specify include directories or targets to use when compiling a given target.

[target_link_directories](https://cmake.org/cmake/help/latest/command/target_link_directories.html)
Add link directories to a target

### Misc
Stack overflow: [How to get FFTW++ working on windows? (for dummies)](https://stackoverflow.com/questions/39675436/how-to-get-fftw-working-on-windows-for-dummies) - Useful in finding out to use `lib` and they refer back to the FFTW docs. But then the example goes heavily into visual studio code setup, which is not what I'm aiming for.



### Maybe useful later
Disable warning using pragma comment in https://stackoverflow.com/questions/29619907/avoid-warning-unreferenced-formal-parameter-with-default-argument


What is CMAKE `find_package`?

https://stackoverflow.com/questions/52255867/adding-a-dll-to-cmake
"As for the dll files (assuming Windows), either set the PATH environment variable or copy the DLL to the directory where the .exe is located."