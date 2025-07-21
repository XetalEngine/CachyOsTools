#!/bin/bash
set -e  # Exit on any error

cd /home/xetal/Desktop/CachyOsTools/qemu_modified/qemu/

mkdir -p build
cd build

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

make -j"$(nproc)"

sudo make install
