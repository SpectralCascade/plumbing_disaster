# plumbing_disaster
Very simplistic randomly-generated pipe fixing game I made a while ago with SDL 2. A build exists on [Itch.io](https://spectralcascade.itch.io/plumbing-disaster)

# Building
I have setup the project to be built for Windows and Android using [soupbuild](https://github.com/SpectralCascade/soupbuild). You must have Android Studio installed to build for Android. You should have mingw32-make installed to build for Windows; I have the mingw toolchain installed via MSYS2.

To build this project, clone or download [soupbuild](https://github.com/SpectralCascade/soupbuild), then run `soupbuild.py` from this project's directory with Python 3.8 or newer.

FYI the main dependencies are SDL2, SDL2_mixer and SDL2_ttf (but these are taken care of by Soupbuild automatically)

# Additional notes
Must be built before it can be run. Expect some minor bugs. Also the generation is completely random. That means there are some pipe configurations that might actually be impossible to solve. You've been warned!
