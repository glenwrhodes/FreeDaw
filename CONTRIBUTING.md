# Contributing to OpenDaw

Thank you for your interest in contributing to OpenDaw! This document explains how to get involved.

## Ways to Contribute

- **Report bugs** — open an issue describing the problem, steps to reproduce, and your system info
- **Suggest features** — open an issue tagged as a feature request
- **Submit pull requests** — fix a bug, improve documentation, or add a feature
- **Improve docs** — typos, clarifications, and better examples are always welcome

## Getting Started

### 1. Fork and clone

```bash
git clone --recurse-submodules https://github.com/<your-username>/AudioMixer.git
cd AudioMixer
```

### 2. Set up the correct JUCE version

Tracktion Engine requires a pinned JUCE commit:

```powershell
cd libs/JUCE
git fetch origin 7c89e11f6b7316c369f3d3f22227c60e816e738b
git checkout 7c89e11f6b7316c369f3d3f22227c60e816e738b
cd ../..
```

### 3. Build

See the [README](README.md#building) for full build instructions. In short:

```powershell
# Configure
cmd /c "`"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat`" x64 && c:\qt\Tools\CMake_64\bin\cmake.exe -B build -G Ninja -DCMAKE_PREFIX_PATH=c:/qt/6.10.2/msvc2022_64 -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_MAKE_PROGRAM=c:/qt/Tools/Ninja/ninja.exe"

# Build
cmd /c "`"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat`" x64 && c:\qt\Tools\CMake_64\bin\cmake.exe --build build"
```

### 4. Create a branch

```bash
git checkout -b my-feature
```

## Pull Request Process

1. **One PR per concern** — keep changes focused. A bug fix and a new feature should be separate PRs.
2. **Build before submitting** — make sure the project compiles without errors.
3. **Describe your changes** — write a clear PR description explaining what changed and why.
4. **Keep commits clean** — use descriptive commit messages. Squash fixup commits before requesting review.

## Coding Conventions

- **C++20** standard
- **Namespace**: all project code lives in the `OpenDaw` namespace
- **JUCE namespace**: JUCE types must be qualified with `juce::` (the global using directive is disabled via `DONT_SET_USING_JUCE_NAMESPACE=1`)
- **Tracktion Engine alias**: use `te::` (defined as `namespace te = tracktion::engine;`)
- **Qt for GUI, Tracktion for audio** — do not use JUCE GUI classes; do not use Qt for audio processing
- **Formatting**: match the style of surrounding code (4-space indentation, braces on their own line for functions)

## Reporting Issues

When filing a bug report, please include:

- Windows version (e.g., Windows 11 23H2)
- Visual Studio version
- Qt version
- Steps to reproduce the issue
- Expected vs. actual behavior
- Any relevant log output or screenshots

## Code of Conduct

Be respectful and constructive. We're all here to make a great free DAW.

## License

By contributing to OpenDaw, you agree that your contributions will be licensed under the [GNU General Public License v3.0](LICENSE).
