# Strange exit codes running app from CLion (missing dlls)
```
C:\Users\g\avva\projects\JUCECmakeRepoPrototype\cmake-build-debug\Apps\mbk\mbk_artefacts\Debug\mbk.exe

Process finished with exit code -1073741515 (0xC0000135)
```

Running `C:\Users\g\avva\projects\JUCECmakeRepoPrototype\cmake-build-debug\Apps\mbk\mbk_artefacts\Debug\mbk.exe` from _Windows File Explorer_ shows that some dlls are missing.

![missing dll](assets/libwinpthread-1.dll-missing.png)

List of dlls missing:
* `libwinpthread-1.dll`
* `libgcc_s_seh-1.dll`
* `libstdc++-6.dll`
* `libfftw3-3.dll`

Switched compiler to visual studio code and deleted `cmake-build-debug`.
Tried building mbk.
_File_ -> _Rebuild CMake Project_
Close CLion
`git clean -xdf`

Basically try a few things to get it to build with the visual studio cpp build system then it will just be the following missing:
* `libfftw3-3.dll`
* `samplerate.dll`

`samplerate.dll` is available in the libsamplerate download dir inside the `bin` dir.
`libfftw3-3.dll` is available in the fftw download dir inside the root of the dir

These need to go next to the `mbk.exe` binary, wherever it is being run from.