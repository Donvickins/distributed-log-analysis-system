#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Check for and install required packages: git, cmake, g++, curl, unzip
REQUIRED_PKGS=(git cmake g++ curl unzip)
MISSING_PKGS=()
for pkg in "${REQUIRED_PKGS[@]}"; do
    if ! command -v $pkg &> /dev/null; then
        MISSING_PKGS+=("$pkg")
    fi
done
if [ ${#MISSING_PKGS[@]} -ne 0 ]; then
    echo "Installing missing packages: ${MISSING_PKGS[*]}"
    sudo apt update && sudo apt install -y "${MISSING_PKGS[@]}"
fi

# Check for vcpkg
if [ ! -d "$HOME/vcpkg" ]; then
    echo "Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg" || { echo "Failed to clone vcpkg."; exit 1; }
fi

# Bootstrap vcpkg
pushd "$HOME/vcpkg" &> /dev/null || { echo "Failed to change directory to vcpkg."; exit 1; }
if [ -f bootstrap-vcpkg.sh ]; then
    ./bootstrap-vcpkg.sh || { echo "Failed to bootstrap vcpkg."; exit 1; }
    ./vcpkg integrate install
else
    echo "bootstrap-vcpkg.sh not found!"; exit 1
fi
popd &> /dev/null || { echo "Failed to return to previous directory."; exit 1; }

# Install dependencies via vcpkg
"$HOME/vcpkg/vcpkg" install --triplet x64-linux --manifest || { echo "vcpkg install failed."; exit 1; }

# Create build directory and run cmake
BUILD_DIR="$SCRIPT_DIR/build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi
pushd "$BUILD_DIR" || { echo "Failed to enter build directory."; exit 1; }
cmake .. || { echo "CMake configuration failed."; exit 1; }
popd


