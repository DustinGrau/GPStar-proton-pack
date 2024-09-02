# VSCode + PlatformIO

This guide will outline how to begin coding and compiling using Visual Studio Code and PlatformIO instead of the ArduinoIDE.

## Prerequisites

Start with downloading the VSCode IDE for your operating system and get the PlatformIO extension added.

- [Visual Studio Code for Windows, macOS, and Linux](https://code.visualstudio.com/download)
- [How to install PlatformIO for VSCode](https://platformio.org/install/ide?install=vscode)

Once PIO is available you can get to the Platforms and Libraries tabs to begin adding support for our typical microcontrollers and to access the same libraries as found in the ArduinoIDE. We need support for the right platforms first, so we'll need to access PlatformIO and begin adding those.

1. Select the PlatformIO tab on the left-hand panel
1. Go to **PIO Home > Platforms**
1. Install the following:
	- **Atmel AVR** for Arduino Nano
	- **Atmel megaAVR** for ATMega 2560
	- **Espressif 32** for ESP32

## Libraries

Before installing a library you'll need a project in context. Alternatively, once a project is configured with libraries they will be downloaded automatically and kept up to date per an associated **platformio.ini** file.

- Go to **PIO Home > Libraries**
	- **ArduinoINA219** by Flavius Bindea (1.1.1+)
	- **CRC32** by Christopher Baker (2.0.0+)
	- **digitalWriteFast** by Armin Joachimsmeyer (1.2.0+)
	- **ezButton** by ArduinoGetStarted.com (1.0.6+)
	- **FastLED** by Daniel Garcia (3.7.4+)
	- **Ramp** by Sylvain Garnavault (0.6.1+)
	- **SafeString** by Matthew Ford "powerbroker2" (4.1.34+)
	- **SerialTransfer** by PowerBroker2 (3.1.3+)
	- **AltSoftSerial** by Paul Stoffregen (1.4.0+)
	- **Simple ht16k33 Library** by Ipaseen (1.0.2+)
	- **Switch** by Abhijit Bose (1.2.7+)


## Command Line Tools

To compile a PlatformIO project via shell script, you will need the "pio" utility which can be installed via [Python v3.11 or higher](https://www.python.org/downloads/).

After installing Python3 run the following:

`pip install platformio`

If you need to upgrade pip that can be done using the following:

`pip install --upgrade pip`

Once the `pio` utility is available, the included scripts in the project's `.github/` folder may be used to compile code.