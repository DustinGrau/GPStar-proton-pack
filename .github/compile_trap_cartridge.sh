#!/bin/bash

# Perform a full compile of all binaries using PlatformIO (pio).
#
# This script compiles only the main ATtiny binary.

BINDIR="../binaries"
SRCDIR="../source"
PROJECT_DIR="$SRCDIR/GhostTrapCartridge"

mkdir -p ${BINDIR}/trap/extras

echo ""

# GhostTrap (ESP32 - Normal)
echo "Building GhostTrap Binary (ATtiny)..."

# Clean the project before building
pio run --project-dir "$PROJECT_DIR" --target clean

# Compile the PlatformIO project
pio run --project-dir "$PROJECT_DIR"

# Check if the build was successful
if [ $? -eq 0 ]; then
  echo "Build succeeded!"
else
  echo "Build failed!"
  exit 1
fi

# Copy the new firmware to the expected binaries directory
if [ -f ${PROJECT_DIR}/.pio/build/attiny1616/firmware.elf ]; then
  mv ${PROJECT_DIR}/.pio/build/attiny1616/firmware.elf ${BINDIR}/trap/GhostTrap-ATtiny.bin
fi
echo "Done."
echo ""

rm -f ${PROJECT_DIR}/include/*.h-e