#!/bin/bash

# Launch the CachyOsTools application
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_PATH="$SCRIPT_DIR/CachyOsTools"

# Check if executable exists in root directory (new build system)
if [ -f "$APP_PATH" ]; then
    exec "$APP_PATH"
else
    # Fallback: try to find executable in build directories
    echo "Executable not found in root directory. Searching build directories..."
    
    # Look for the most recent build
    for build_dir in "$SCRIPT_DIR"/build/Desktop-*; do
        if [ -d "$build_dir" ] && [ -f "$build_dir/CachyOsTools" ]; then
            echo "Found executable in: $build_dir"
            exec "$build_dir/CachyOsTools"
        fi
    done
    
    echo "Error: CachyOsTools executable not found!"
    echo "Please run ./build.sh first to build the application."
    exit 1
fi 