#!/bin/bash

# CachyOsTools Build Script 2 - GCC + Make
# This script builds the Qt application using CMake and Make with GCC
# Compatible with Manjaro, Ubuntu, Debian, and other distributions

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_status "Building CachyOsTools (GCC + Make) from: $SCRIPT_DIR"

# Check if we're in the project root
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# Default build type
BUILD_TYPE=${1:-Debug}
BUILD_DIR="build/Desktop-$BUILD_TYPE-GCC"

print_status "Build type: $BUILD_TYPE"
print_status "Build directory: $BUILD_DIR"
print_status "Compiler: GCC"
print_status "Generator: Make"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    print_status "Creating build directory: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Check if CMake cache exists and is valid
if [ ! -f "CMakeCache.txt" ] || [ ! -f "Makefile" ]; then
    print_status "Configuring CMake with GCC..."
    cmake ../.. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -G "Unix Makefiles"
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed!"
        exit 1
    fi
    print_success "CMake configuration completed"
else
    print_status "Using existing CMake cache"
fi

# Build the project
print_status "Building with Make..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    print_error "Build failed!"
    exit 1
fi

print_success "Build completed successfully!"

# Check if executable exists
if [ -f "CachyOsTools" ]; then
    print_success "Executable created: $BUILD_DIR/CachyOsTools"
    print_status "You can run the application with: ./CachyOsTools"
else
    print_warning "Executable not found. Check build output for errors."
fi

# Return to original directory
cd "$SCRIPT_DIR"

print_status "Build script 2 (GCC + Make) completed" 