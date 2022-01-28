# plumbing_disaster
Very simplistic randomly-generated pipe fixing game I made a while ago with SDL 2. A build exists on [Itch.io](https://spectralcascade.itch.io/plumbing-disaster)

# Pre-requisites
All these components are listed on the [SDL website](https://www.libsdl.org/download-2.0.php)
* SDL 2
* SDL_Mixer
* SDL_TTF

# Building
I have setup the project to be built for Windows and Android using [soupbuild](https://github.com/SpectralCascade/soupbuild). You must have Android Studio installed to build for Android. You should have Make installed to build for Windows - you will need to change the path to point to your installation of Make in the line
`"cmd /c C:\\msys64\\mingw64\\bin\\mingw32-make.exe {mode} -j 8"` under `"build"` task steps in the `build.soup` file.

To build this project, clone or download [soupbuild](https://github.com/SpectralCascade/soupbuild), then run `soupbuild.py` from this project's directory (containing `build.soup`).

# Additional notes
Must be built before it can be run, with relevant SDL 2 dev files etc. Expect some minor bugs. Also the generation is completely random. That means there are some pipe configurations that might actually be impossible to solve. You've been warned!
