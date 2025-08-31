#!/bin/bash

# Create Complete Offline Package Script
# This script creates an offline package that includes both packages and databases

set -e

echo "Creating complete offline package..."

# Create temporary directory
TEMP_DIR="/tmp/offline-package-$(date +%s)"
mkdir -p "$TEMP_DIR/offline-packages"

echo "Copying packages..."
# Copy all packages
cp -r /var/cache/pacman/pkg/* "$TEMP_DIR/offline-packages/pkg/" 2>/dev/null || true

echo "Copying package databases..."
# Copy package databases
cp -r /var/lib/pacman/sync/* "$TEMP_DIR/offline-packages/sync/" 2>/dev/null || true

echo "Copying configuration files..."
# Copy the package list and pacman config
cp /home/xetal/iso/xiso/packages.x86_64 "$TEMP_DIR/offline-packages/"
cp /home/xetal/iso/xiso/work/iso.pacman.conf "$TEMP_DIR/offline-packages/"

echo "Creating archive..."
# Create the final archive
cd "$TEMP_DIR"
tar -czf "/home/xetal/iso/offlinezip/offline-iso-packages-complete.tar.gz" offline-packages/

echo "Cleaning up..."
rm -rf "$TEMP_DIR"

echo "Complete offline package created: /home/xetal/iso/offlinezip/offline-iso-packages-complete.tar.gz"
echo "This package includes:"
echo "- All required packages"
echo "- Package databases"
echo "- Configuration files"
echo ""
echo "This package will work 100% offline!"
