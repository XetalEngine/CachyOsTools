#!/bin/bash
set -e  # Exit on any error

# Navigate to QEMU source directory
cd /home/xetal/Desktop/CachyOsTools/qemu_modified/qemu/

# Create and enter the build directory
mkdir -p build
cd build

# Configure the build
../configure \
  --prefix=/opt/qemu-stealth \
  --target-list=x86_64-softmmu \
  --enable-kvm \
  --enable-spice \
  --enable-libusb \
  --enable-usb-redir \
  --enable-virtfs \
  --enable-tools \
  --disable-werror

# Compile using all available cores
make -j"$(nproc)"

# Install to /opt/qemu-stealth (requires root)
sudo make install
