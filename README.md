# Mavish Game

A first-person 3D game built with [raylib](https://www.raylib.com/).

## Features

- First-person camera with noclip (flying) and walking modes
- Physics with gravity, collision detection, and jumping
- Settings menu with customizable FPS, FOV, sensitivity, and move speed
- Multiple window modes (Windowed, Borderless, Exclusive Fullscreen)
- Performance/debug overlay (F3)

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look around |
| Space | Jump (walking mode) / Fly up (noclip) |
| Ctrl | Fly down (noclip mode) |
| Shift | Sprint |
| V | Toggle noclip/walking mode |
| ESC | Settings menu |
| F3 | Debug/performance overlay |
| F11 | Toggle fullscreen |

## Prerequisites

### All Platforms
- [CMake](https://cmake.org/download/) 3.16 or higher
- [vcpkg](https://github.com/microsoft/vcpkg) package manager

### Windows
- Visual Studio 2022 with C++ workload
- Or: Visual Studio Build Tools 2022

### Linux (Ubuntu/Debian)
```bash
# Build tools
sudo apt install build-essential cmake ninja-build

# raylib dependencies
sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev libgl1-mesa-dev
```

## Setup

### 1. Install vcpkg (if not already installed)

**Windows:**
```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
# Add to system environment variables:
setx VCPKG_ROOT "C:\vcpkg"
```

**Linux:**
```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg
./bootstrap-vcpkg.sh
# Add to ~/.bashrc:
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc
source ~/.bashrc
```

### 2. Clone and Build

```bash
git clone <repo-url> mavish
cd mavish
```

## Build Commands

### Windows (PowerShell)

| Command | Description |
|---------|-------------|
| `.\build.bat` | Build release |
| `.\build.bat run` | Build + run game |
| `.\build.bat debug` | Build debug |
| `.\build.bat debug run` | Build debug + run |
| `.\build.bat clean` | Clean rebuild |
| `.\build.bat clean run` | Clean rebuild + run |

### Linux / macOS (Bash)

| Command | Description |
|---------|-------------|
| `./build.sh` | Build release |
| `./build.sh run` | Build + run game |
| `./build.sh debug` | Build debug |
| `./build.sh debug run` | Build debug + run |
| `./build.sh clean` | Clean rebuild |
| `./build.sh clean run` | Clean rebuild + run |

## Manual CMake Commands

If you prefer manual CMake commands:

**Windows:**
```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
.\build\Release\MavishGame.exe
```

**Linux:**
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/MavishGame
```

## Project Structure

```
mavish/
├── src/
│   ├── main.cpp      # Main game code
│   └── raygui.h      # GUI library (single header)
├── resources/        # Game assets (textures, models, etc.)
├── build.bat         # Windows build script
├── build.sh          # Linux build script
├── CMakeLists.txt    # CMake configuration
├── CMakePresets.json # CMake presets for IDE integration
├── vcpkg.json        # vcpkg dependencies
└── README.md
```

## License

MIT License - See LICENSE file for details.
