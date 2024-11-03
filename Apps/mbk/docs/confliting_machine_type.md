```
BTrack.cpp.obj : error LNK2019: unresolved external symbol _src_simple referenced in function "private: void __thiscall BTrack::resampleOnsetDetectionFunction(void)" (?resampleOnsetDetectionFunction@BTrack@@AAEXXZ)
C:\Users\g\avva\projects\JUCECmakeRepoPrototype\Apps\mbk\..\..\..\dependencies\fftw3.3.10-dll64\libfftw3-3.lib : warning LNK4272: library machine type 'x64' conflicts with target machine type 'x86'
C:\Users\g\avva\projects\JUCECmakeRepoPrototype\Apps\mbk\..\..\..\dependencies\libsamplerate-0.2.2-win64\lib\samplerate.lib : warning LNK4272: library machine type 'x64' conflicts with target machine type 'x86'
Apps\mbk\mbk_artefacts\Debug\mbk.exe : fatal error LNK1120: 6 unresolved externals
ninja: build stopped: subcommand failed.
```

Change architecture to amd64 in the build config.