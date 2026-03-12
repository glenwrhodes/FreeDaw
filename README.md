# FreeDaw

A free, open-source Digital Audio Workstation built with Qt6 and Tracktion Engine.

## Features

- Multi-track audio timeline with drag-and-drop
- Grid snapping (off / quarter-beat / half-beat / beat / bar)
- Built-in effects: Reverb, EQ, Compressor, Delay, Chorus, Phaser, Low Pass, Pitch Shift
- Mixer with per-track volume fader, pan knob, mute/solo, level meters
- Audio recording from input devices
- File browser with audio file filtering
- VST3 plugin hosting
- Transport controls with BPM and time signature
- Project save/load (Tracktion Edit format)

## Prerequisites

- **Qt 6.10+** with MinGW 64-bit kit
- **CMake 3.22+**
- **Git**

## Building

```bash
# Clone with submodules
git clone --recurse-submodules <repo-url>
cd AudioMixer

# Configure (adjust Qt path if needed)
cmake -B build -G Ninja ^
  -DCMAKE_PREFIX_PATH=c:/qt/6.10.2/mingw_64 ^
  -DCMAKE_C_COMPILER=c:/qt/Tools/mingw1310_64/bin/gcc.exe ^
  -DCMAKE_CXX_COMPILER=c:/qt/Tools/mingw1310_64/bin/g++.exe ^
  -DCMAKE_MAKE_PROGRAM=c:/qt/Tools/Ninja/ninja.exe

# Build
cmake --build build
```

## Usage

1. Launch FreeDaw
2. Use the file browser (left panel) to navigate to audio files
3. Drag audio files onto timeline tracks
4. Press Play to hear your arrangement
5. Use the mixer (bottom panel) to adjust volume, pan, and add effects
6. Save your project via File > Save

## License

This project uses Tracktion Engine (GPLv3) and JUCE (AGPLv3).
FreeDaw is licensed under GPLv3.
