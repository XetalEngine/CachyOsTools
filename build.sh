#!/bin/bash

# CachyOsTools Build Script
# This script builds the Qt application using CMake with various compiler options

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
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

print_cyan() {
    echo -e "${CYAN}$1${NC}"
}

# Function to check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Function to print dependencies for different distributions
print_dependencies() {
    local compiler=$1
    local build_system=$2
    
    print_cyan "=== DEPENDENCIES FOR $compiler + $build_system ==="
    echo ""
    
    case $compiler in
        "GCC")
            print_cyan "Arch/Manjaro:"
            echo "sudo pacman -S gcc make cmake qt6-base qt6-tools"
            echo ""
            print_cyan "Ubuntu/Debian:"
            echo "sudo apt install gcc g++ make cmake qt6-base-dev qt6-tools-dev"
            echo ""
            print_cyan "Fedora:"
            echo "sudo dnf install gcc gcc-c++ make cmake qt6-qtbase-devel qt6-qttools-devel"
            ;;
        "Clang")
            print_cyan "Arch/Manjaro:"
            echo "sudo pacman -S clang make cmake qt6-base qt6-tools"
            echo ""
            print_cyan "Ubuntu/Debian:"
            echo "sudo apt install clang make cmake qt6-base-dev qt6-tools-dev"
            echo ""
            print_cyan "Fedora:"
            echo "sudo dnf install clang make cmake qt6-qtbase-devel qt6-qttools-devel"
            ;;
    esac
    
    if [ "$build_system" = "Ninja" ]; then
        echo ""
        print_cyan "Additional Ninja dependency:"
        print_cyan "Arch/Manjaro:"
        echo "sudo pacman -S ninja"
        print_cyan "Ubuntu/Debian:"
        echo "sudo apt install ninja-build"
        print_cyan "Fedora:"
        echo "sudo dnf install ninja-build"
    fi
    
    echo ""
    print_cyan "=== END DEPENDENCIES ==="
    echo ""
}

# Function to select compiler and build system
select_build_config() {
    print_cyan "=== CachyOsTools Build Configuration ==="
    echo ""
    print_cyan "Available build configurations:"
    echo "1) GCC + Make (Recommended for Manjaro/Ubuntu/Debian)"
    echo "2) Clang + Make (Recommended for Arch Linux)"
    echo "3) GCC + Ninja (Fastest builds, requires Ninja)"
    echo "4) System Default + Ninja (Original configuration)"
    echo ""
    
    while true; do
        read -p "Select build configuration (1-4): " choice
        case $choice in
            1)
                COMPILER="GCC"
                BUILD_SYSTEM="Make"
                CMAKE_GENERATOR="Unix Makefiles"
                CMAKE_CXX_COMPILER="g++"
                CMAKE_C_COMPILER="gcc"
                BUILD_DIR_SUFFIX="GCC"
                break
                ;;
            2)
                COMPILER="Clang"
                BUILD_SYSTEM="Make"
                CMAKE_GENERATOR="Unix Makefiles"
                CMAKE_CXX_COMPILER="clang++"
                CMAKE_C_COMPILER="clang"
                BUILD_DIR_SUFFIX="Clang"
                break
                ;;
            3)
                COMPILER="GCC"
                BUILD_SYSTEM="Ninja"
                CMAKE_GENERATOR="Ninja"
                CMAKE_CXX_COMPILER="g++"
                CMAKE_C_COMPILER="gcc"
                BUILD_DIR_SUFFIX="GCC-Ninja"
                break
                ;;
            4)
                COMPILER="System Default"
                BUILD_SYSTEM="Ninja"
                CMAKE_GENERATOR="Ninja"
                CMAKE_CXX_COMPILER=""
                CMAKE_C_COMPILER=""
                BUILD_DIR_SUFFIX=""
                break
                ;;
            *)
                print_error "Invalid choice. Please select 1-4."
                ;;
        esac
    done
    
    echo ""
    print_success "Selected: $COMPILER + $BUILD_SYSTEM"
    echo ""
    
    # Select build type
    print_cyan "=== Build Type Selection ==="
    echo ""
    print_cyan "Available build types:"
    echo "1) Debug (Default - includes debug symbols, slower performance)"
    echo "2) Release (Optimized - faster performance, smaller executable)"
    echo ""
    
    while true; do
        read -p "Select build type (1-2): " build_choice
        case $build_choice in
            1)
                BUILD_TYPE="Debug"
                break
                ;;
            2)
                BUILD_TYPE="Release"
                break
                ;;
            *)
                print_error "Invalid choice. Please select 1-2."
                ;;
        esac
    done
    
    echo ""
    print_success "Selected build type: $BUILD_TYPE"
    echo ""
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    
    # Check CMake
    if ! command_exists cmake; then
        missing_deps+=("cmake")
    fi
    
    # Check compiler
    case $COMPILER in
        "GCC")
            if ! command_exists gcc; then
                missing_deps+=("gcc")
            fi
            if ! command_exists g++; then
                missing_deps+=("g++")
            fi
            ;;
        "Clang")
            if ! command_exists clang; then
                missing_deps+=("clang")
            fi
            if ! command_exists clang++; then
                missing_deps+=("clang++")
            fi
            ;;
    esac
    
    # Check build system
    case $BUILD_SYSTEM in
        "Make")
            if ! command_exists make; then
                missing_deps+=("make")
            fi
            ;;
        "Ninja")
            if ! command_exists ninja; then
                missing_deps+=("ninja")
            fi
            ;;
    esac
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        echo ""
        print_dependencies "$COMPILER" "$BUILD_SYSTEM"
        exit 1
    fi
    
    print_success "All dependencies found!"
    echo ""
}

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_status "Building CachyOsTools from: $SCRIPT_DIR"

# Check if we're in the project root
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# Select build configuration
select_build_config

# Check dependencies
check_dependencies

# Build directory based on selected configuration
BUILD_DIR="build/Desktop-$BUILD_TYPE-$BUILD_DIR_SUFFIX"

print_status "Build type: $BUILD_TYPE"
print_status "Build directory: $BUILD_DIR"
print_status "Compiler: $COMPILER"
print_status "Build system: $BUILD_SYSTEM"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    print_status "Creating build directory: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Check if CMake cache exists and is valid
build_file=""
case $BUILD_SYSTEM in
    "Make")
        build_file="Makefile"
        ;;
    "Ninja")
        build_file="build.ninja"
        ;;
esac

if [ ! -f "CMakeCache.txt" ] || [ ! -f "$build_file" ]; then
    print_status "Configuring CMake..."
    
    cmake_cmd="cmake ../.. -DCMAKE_BUILD_TYPE=\"$BUILD_TYPE\" -G \"$CMAKE_GENERATOR\""
    
    if [ -n "$CMAKE_CXX_COMPILER" ]; then
        cmake_cmd="$cmake_cmd -DCMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER"
    fi
    
    if [ -n "$CMAKE_C_COMPILER" ]; then
        cmake_cmd="$cmake_cmd -DCMAKE_C_COMPILER=$CMAKE_C_COMPILER"
    fi
    
    eval $cmake_cmd
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed!"
        exit 1
    fi
    print_success "CMake configuration completed"
else
    print_status "Using existing CMake cache"
fi

# Build the project
print_status "Building with $BUILD_SYSTEM..."
case $BUILD_SYSTEM in
    "Make")
        make -j$(nproc)
        ;;
    "Ninja")
        ninja
        ;;
esac

if [ $? -ne 0 ]; then
    print_error "Build failed!"
    exit 1
fi

print_success "Build completed successfully!"

# Check if executable exists and copy to root directory
if [ -f "CachyOsTools" ]; then
    print_success "Executable created: $BUILD_DIR/CachyOsTools"
    
    # Copy executable to root directory
    cp "CachyOsTools" "$SCRIPT_DIR/"
    print_success "Executable copied to root directory: $SCRIPT_DIR/CachyOsTools"
    print_status "You can run the application with: ./CachyOsTools"
else
    print_warning "Executable not found. Check build output for errors."
fi

# Return to original directory
cd "$SCRIPT_DIR"

print_status "Build script completed" 