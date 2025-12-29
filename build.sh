#!/bin/bash

# ============================================
# Mavish Game - Linux Build Script
# ============================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for VCPKG_ROOT environment variable
if [ -z "$VCPKG_ROOT" ]; then
    echo -e "${RED}ERROR: VCPKG_ROOT environment variable is not set.${NC}"
    echo ""
    echo "Please install vcpkg and set VCPKG_ROOT:"
    echo "  1. git clone https://github.com/microsoft/vcpkg.git ~/vcpkg"
    echo "  2. cd ~/vcpkg && ./bootstrap-vcpkg.sh"
    echo "  3. Add to ~/.bashrc: export VCPKG_ROOT=~/vcpkg"
    echo "  4. Restart your terminal"
    echo ""
    echo "Also install Linux dependencies for raylib:"
    echo "  sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev libgl1-mesa-dev"
    exit 1
fi

# Default values
BUILD_TYPE="Release"
CLEAN_BUILD=0
RUN_AFTER=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        debug|Debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        release|Release)
            BUILD_TYPE="Release"
            shift
            ;;
        clean)
            CLEAN_BUILD=1
            shift
            ;;
        run)
            RUN_AFTER=1
            shift
            ;;
        *)
            echo "Unknown argument: $1"
            echo "Usage: ./build.sh [debug|release] [clean] [run]"
            exit 1
            ;;
    esac
done

# Clean build directory if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf build
fi

# Create build directory
mkdir -p build

# Configure with CMake
echo ""
echo -e "${YELLOW}Configuring CMake ($BUILD_TYPE)...${NC}"
cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -G "Ninja"

# Build
echo ""
echo -e "${YELLOW}Building...${NC}"
cmake --build build --config "$BUILD_TYPE"

echo ""
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}Build successful!${NC}"
echo -e "${GREEN}Executable: build/MavishGame${NC}"
echo -e "${GREEN}============================================${NC}"

# Run if requested
if [ $RUN_AFTER -eq 1 ]; then
    echo ""
    echo -e "${YELLOW}Starting game...${NC}"
    ./build/MavishGame
fi