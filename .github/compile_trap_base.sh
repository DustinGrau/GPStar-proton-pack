#!/bin/bash

# Perform a full compile of all binaries using PlatformIO (pio).
#
# This script compiles only the main ESP32 binary.

BINDIR="../binaries"
SRCDIR="../source"

mkdir -p ${BINDIR}/trap/extras

# Current build timestamp to be reflected in the build for ESP32.
TIMESTAMP=$(date +"%Y%m%d%H%M%S")

echo ""

# Set the project directory based on the source folder
PROJECT_DIR="$SRCDIR/GhostTrapBase"

# Update date of compilation
echo "Updating Build Timestamp:" $TIMESTAMP
sed -i -e 's/\(String build_date = "\)[^"]*\(";\)/\1'"$TIMESTAMP"'\2/' ${PROJECT_DIR}/include/Configuration.h

# GhostTrap (ESP32 - Normal)
echo "Building GhostTrap Binary (ESP32)..."

# Clean the project before building
pio run --project-dir "$PROJECT_DIR" --target clean

# Compile the PlatformIO project
pio run --project-dir "$PROJECT_DIR" | grep -iv Retrieved

rm -f ${PROJECT_DIR}/include/*.h-e

if [ -f ${PROJECT_DIR}/.pio/build/esp32s3mini/firmware.bin ]; then
  mv ${PROJECT_DIR}/.pio/build/esp32s3mini/firmware.bin ${BINDIR}/trap/GhostTrap-ESP32.bin
fi
if [ -f ${PROJECT_DIR}/.pio/build/esp32s3mini/bootloader.bin ]; then
  mv ${PROJECT_DIR}/.pio/build/esp32s3mini/bootloader.bin ${BINDIR}/trap/extras/GhostTrap-ESP32-Bootloader.bin
fi
if [ -f ${PROJECT_DIR}/.pio/build/esp32s3mini/partitions.bin ]; then
  mv ${PROJECT_DIR}/.pio/build/esp32s3mini/partitions.bin ${BINDIR}/trap/extras/GhostTrap-ESP32-Partitions.bin
fi
echo "Done."
echo ""
