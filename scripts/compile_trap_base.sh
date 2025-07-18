#!/bin/bash

# Perform a full compile of all binaries using PlatformIO (pio).
#
# This script compiles only the main ESP32 binary.

BINDIR="../binaries"
SRCDIR="../source"
PROJECT_DIR="$SRCDIR/GhostTrapBase"

mkdir -p ${BINDIR}/trap/extras

# Current build timestamp and major version to be reflected in the build for ESP32.
MJVER="${MJVER:="V6"}"
TIMESTAMP="${TIMESTAMP:=$(date +"%Y%m%d%H%M%S")}"

# Update date of compilation
echo "Setting Build Timestamp: ${MJVER}_${TIMESTAMP}"
sed -i -e 's/\(String build_date = "\)[^"]*\(";\)/\1'"${MJVER}_${TIMESTAMP}"'\2/' ${PROJECT_DIR}/include/Configuration.h

echo ""

# GhostTrap (ESP32 - Normal)
echo "Building GhostTrap Binary (ESP32)..."

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
if [ -f ${PROJECT_DIR}/.pio/build/esp32s3mini/firmware.bin ]; then
  mv ${PROJECT_DIR}/.pio/build/esp32s3mini/firmware.bin ${BINDIR}/trap/GhostTrap-Base.bin
fi
if [ -f ${PROJECT_DIR}/.pio/build/esp32s3mini/bootloader.bin ]; then
  mv ${PROJECT_DIR}/.pio/build/esp32s3mini/bootloader.bin ${BINDIR}/trap/extras/GhostTrap-Base-Bootloader.bin
fi
if [ -f ${PROJECT_DIR}/.pio/build/esp32s3mini/partitions.bin ]; then
  mv ${PROJECT_DIR}/.pio/build/esp32s3mini/partitions.bin ${BINDIR}/trap/extras/GhostTrap-Base-Partitions.bin
fi
echo "Done."
echo ""

rm -f ${PROJECT_DIR}/include/*.h-e
