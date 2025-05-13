#!/bin/bash

# Perform a full compile of all binaries using PlatformIO (pio).
#
# This script compiles only the main ATtiny binary.

BINDIR="../binaries"
SRCDIR="../source"

mkdir -p ${BINDIR}/trap/extras

# Current build timestamp to be reflected in the build for ESP32.
TIMESTAMP=$(date +"%Y%m%d%H%M%S")

echo ""

# Set the project directory based on the source folder
PROJECT_DIR="$SRCDIR/GhostTrapCartridge"

# GhostTrap (ESP32 - Normal)
echo "Building GhostTrap Binary (ATtiny)..."

# Clean the project before building
pio run --project-dir "$PROJECT_DIR" --target clean

# Compile the PlatformIO project
pio run --project-dir "$PROJECT_DIR"

if [ -f ${PROJECT_DIR}/.pio/build/attiny1616/firmware.bin ]; then
  mv ${PROJECT_DIR}/.pio/build/attiny1616/firmware.bin ${BINDIR}/trap/GhostTrap-ATtiny.bin
fi
echo "Done."
echo ""
