# CachyOsTools Build Scripts

This project includes multiple build scripts for different compiler and build system combinations to ensure compatibility across various Linux distributions.

## Available Build Scripts

### 1. `build.sh` (Original)
- **Compiler**: System default
- **Build System**: Ninja
- **Best for**: Systems with Ninja pre-installed
- **Usage**: `./build.sh [Debug|Release]`

### 2. `build2.sh` (GCC + Make)
- **Compiler**: GCC (gcc/g++)
- **Build System**: Make
- **Best for**: Manjaro, Ubuntu, Debian, Fedora
- **Dependencies**: `gcc`, `g++`, `make`, `cmake`
- **Usage**: `./build2.sh [Debug|Release]`

### 3. `build3.sh` (Clang + Make)
- **Compiler**: Clang (clang/clang++)
- **Build System**: Make
- **Best for**: Arch Linux, Manjaro, systems with Clang
- **Dependencies**: `clang`, `clang++`, `make`, `cmake`
- **Usage**: `./build3.sh [Debug|Release]`

### 4. `build4.sh` (GCC + Ninja)
- **Compiler**: GCC (gcc/g++)
- **Build System**: Ninja
- **Best for**: Systems with Ninja available
- **Dependencies**: `gcc`, `g++`, `ninja`, `cmake`
- **Usage**: `./build4.sh [Debug|Release]`

## Installation Dependencies

### For build2.sh (GCC + Make):
```bash
# Arch/Manjaro
sudo pacman -S gcc make cmake qt6-base qt6-tools

# Ubuntu/Debian
sudo apt install gcc g++ make cmake qt6-base-dev qt6-tools-dev

# Fedora
sudo dnf install gcc gcc-c++ make cmake qt6-qtbase-devel qt6-qttools-devel
```

### For build3.sh (Clang + Make):
```bash
# Arch/Manjaro
sudo pacman -S clang make cmake qt6-base qt6-tools

# Ubuntu/Debian
sudo apt install clang make cmake qt6-base-dev qt6-tools-dev

# Fedora
sudo dnf install clang make cmake qt6-qtbase-devel qt6-qttools-devel
```

### For build4.sh (GCC + Ninja):
```bash
# Arch/Manjaro
sudo pacman -S gcc ninja cmake qt6-base qt6-tools

# Ubuntu/Debian
sudo apt install gcc g++ ninja-build cmake qt6-base-dev qt6-tools-dev

# Fedora
sudo dnf install gcc gcc-c++ ninja-build cmake qt6-qtbase-devel qt6-qttools-devel
```

## Usage Examples

### Debug Build (Default):
```bash
./build2.sh
./build3.sh
./build4.sh
```

### Release Build:
```bash
./build2.sh Release
./build3.sh Release
./build4.sh Release
```

### Clean Build (remove build directory first):
```bash
rm -rf build/
./build2.sh
```

## Troubleshooting

### Common Issues:

1. **"CMake not found"**
   - Install CMake: `sudo pacman -S cmake` (Arch) or `sudo apt install cmake` (Ubuntu)

2. **"Qt6 not found"**
   - Install Qt6 development packages as shown in dependencies above

3. **"Compiler not found"**
   - Install the appropriate compiler (gcc/g++ or clang/clang++)

4. **"Ninja not found"** (for build.sh and build4.sh)
   - Install Ninja: `sudo pacman -S ninja` (Arch) or `sudo apt install ninja-build` (Ubuntu)
   - Or use build2.sh/build3.sh which use Make instead

### Build Directory Structure:
```
build/
├── Desktop-Debug/          # Original build.sh
├── Desktop-Debug-GCC/      # build2.sh
├── Desktop-Debug-Clang/    # build3.sh
└── Desktop-Debug-GCC-Ninja/ # build4.sh
```

## Recommendations

- **For Manjaro/Arch**: Try `build3.sh` first (Clang), then `build2.sh` (GCC)
- **For Ubuntu/Debian**: Try `build2.sh` first (GCC + Make)
- **For Fedora**: Try `build2.sh` first (GCC + Make)
- **For systems with Ninja**: Use `build4.sh` for faster builds

## Performance Notes

- **Ninja builds** are generally faster than Make builds
- **Clang** may provide better error messages and warnings
- **GCC** is more widely available and stable
- **Release builds** are optimized and smaller than Debug builds 