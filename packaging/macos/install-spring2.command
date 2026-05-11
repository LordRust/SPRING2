#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_BIN="$SCRIPT_DIR/spring2"
TARGET_DIR="/usr/local/bin"
TARGET_BIN="$TARGET_DIR/spring2"

if [ ! -f "$SOURCE_BIN" ]; then
  echo "Unable to locate spring2 next to this installer script."
  exit 1
fi

mkdir -p "$TARGET_DIR"

if cp "$SOURCE_BIN" "$TARGET_BIN" 2>/dev/null; then
  chmod +x "$TARGET_BIN"
else
  echo "Installing to $TARGET_DIR requires elevated permissions."
  sudo cp "$SOURCE_BIN" "$TARGET_BIN"
  sudo chmod +x "$TARGET_BIN"
fi

echo
echo "SPRING2 installed to $TARGET_BIN"
echo "Run 'spring2 --version' to verify the installation."
