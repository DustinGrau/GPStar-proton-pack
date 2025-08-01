# eFuse Burning

In order to make use of certain GPIO pins (specifically 39-44) on the `ESP32-S3-WROOM` chips it is necessary to "burn" certain eFuses on the hardware. These are 1-time settable, PERMANENT changes to the hardware which affect its operation.

## Burning

Fuses are burned using the aptly-named `espefuse.py` (or the compiled binary version, in the case of Windows x64). For Linux and MacOS use please see the `source/burn_efusts_esp32s3.sh` script for usage instructions.

## eFuses

The two fuses we need to burn along with their hex values are:

| eFuse Name = Value | Description |
|----|----|
| `UART_PRINT_CONTROL = 0b3` | Setting `UART_PRINT_CONTROL` to 3 disables printing debug messages to `UART0` on boot. This is required in order to use `GPIO43` (TX0) and `GPIO44` (RX0) by not writing console data via these pins. |
| `DIS_PAD_JTAG = 0b1` | Setting `DIS_PAD_JTAG` to 1 disables the JTAG engine at boot from using the physical `JTAG` pins and routes all JTAG functionality through the `USB-CDC` engine instead (via DFU mode), freeing `GPIO 39~42`. |

Important to keep in mind when it comes to setting fuses is that you want to do as few writes as possible during that procedure. Thus `espefuse.py` includes a "batching" feature where you can pass multiple eFuses in a single command and the program will then do the write in a single pass rather than one write per fuse.