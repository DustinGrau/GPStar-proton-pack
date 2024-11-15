#!/bin/bash

# Configuration variables
CHIP="esp32s3"
PORT="/dev/cu.usbmodem146101"   # Change as needed
BAUD="460800"
BOOTLOADER="./.pio/build/esp32s3mini/bootloader.bin"
PARTITION_TABLE="./.pio/build/esp32s3mini/partitions.bin"
FIRMWARE="./.pio/build/esp32s3mini/firmware.bin"
BOOTLOADER_OFFSET="0x0"
PARTITION_OFFSET="0x8000"
FIRMWARE_OFFSET="0x10000"
FLASH_MODE="dio"           # Flash mode: qio, qout, dio, dout
FLASH_FREQ="40m"           # Flash frequency: 40m or 80m
FLASH_SIZE="detect"        # Can specify size explicitly, e.g., 4MB

# Ensure esptool.py is installed
if ! command -v esptool.py &> /dev/null; then
    echo "esptool.py not found! Install it with 'pip install esptool'."
    exit 1
fi

# Execute esptool.py with the specified options
esptool.py --chip "$CHIP" --port "$PORT" --baud "$BAUD" --no-stub \
    --before "default_reset" --after "hard_reset" \
    write_flash --flash_mode "$FLASH_MODE" --flash_freq "$FLASH_FREQ" --flash_size "$FLASH_SIZE" \
    "$BOOTLOADER_OFFSET" "$BOOTLOADER" \
    "$PARTITION_OFFSET" "$PARTITION_TABLE" \
    "$FIRMWARE_OFFSET" "$FIRMWARE"

# Exit with the status of the last command
if [ $? -eq 0 ]; then
    echo "Flashing completed successfully!"
else
    echo "Flashing failed. Please check the connection and try again."
fi
