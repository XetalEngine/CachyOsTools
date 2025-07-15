#!/bin/bash

# 1. Launch the app as your user (Debug build, relative path)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_PATH="$SCRIPT_DIR/build/Desktop-Debug/CachyOsTools"

exec "$APP_PATH" 