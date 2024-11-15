Import("env")
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

env.Replace(
    UPLOADER="esptool.py",
    UPLOADERFLAGS=[
        "--chip", "esp32s3",
        "--port", "$UPLOAD_PORT",
        "--baud", "$UPLOAD_SPEED",
        "--before", "default_reset", "--after", "hard_reset", "--no-stub",
        "write_flash", "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "detect",
        "0x0", "bootloader.bin",
        "0x8000", "partition-table.bin",
        "0x10000", "$SOURCE"
    ]
)

env.Append(UPLOADERFLAGS=["--verify"])
