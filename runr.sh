#!/bin/bash

# 1. Start polkit agent if not running
if ! pgrep -u "$USER" polkit-kde-authentication-agent-1 > /dev/null; then
    /usr/lib/polkit-kde-authentication-agent-1 &
    sleep 1
fi

# 2. Launch the app as your user (Release build, relative path)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_PATH="$SCRIPT_DIR/build/Desktop-Release/CachyOsTools"

if [ ! -x "$APP_PATH" ]; then
    echo "[ERROR] Release build not found at $APP_PATH. Please build it first (./build.sh Release)."
    exit 1
fi

exec "$APP_PATH" 