Import("env")
import subprocess

def custom_upload(source, target, env):
    upload_port = env.subst("$UPLOAD_PORT")
    firmware_path = str(source[0])
    subprocess.run([
        "esptool.py", "--chip", "esp32s3", "--port", upload_port, "--baud", "460800", "--no-stub",
        "write_flash", "0x0", "bootloader.bin", "0x8000", "partition-table.bin", "0x10000", firmware_path
    ])

env.Replace(UPLOADCMD=custom_upload)
