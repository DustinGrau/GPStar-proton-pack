/**
 *   GPStar Stream Effects - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2024-2025 Dustin Grau <dustin.grau@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

/*
 * As an alternative to the standard ESP32 dev board is the Waveshare ESP32-S3 Mini:
 * https://www.waveshare.com/wiki/ESP32-S3-Zero
 */

/*
 * Pin for Addressable LEDs
 * 50 LEDs per Meter: https://a.co/d/dlDyCkz
 */
#define DEVICE_LED_PIN 4
#define DEVICE_MAX_LEDS 500 // Set a hard max for allocating the array of LEDs
uint16_t i_num_leds = 250; // Default is 50 LEDs per meter, with a length of 5 meters (eg. 250)
bool b_grb_leds = false; // Denotes whether to use GRB ordering for LEDs.
CRGB device_leds[DEVICE_MAX_LEDS];

/*
 * Define Color Palettes
 */
CRGBPalette16 paletteWhite;
CRGBPalette16 paletteProton;
CRGBPalette16 paletteSlime;
CRGBPalette16 paletteStasis;
CRGBPalette16 paletteMeson;
CRGBPalette16 paletteSpectral;
CRGBPalette16 paletteHalloween;
CRGBPalette16 paletteChristmas;
CRGBPalette16 cp_StreamPalette; // Current color palette in use.
static const uint8_t i_palette_count = 8; // Total number of palettes available.
static const uint16_t i_selftest_interval = 2000; // 2 seconds between palette changes.
millisDelay ms_selftest_cycle; // Timer for self-test cycling using an interval.
uint8_t i_selftest_palette = 0; // Current palette index for cycling in self-test.

/*
 * Addressable LED Devices
 */
enum device {
  PRIMARY_LED
};

/**
 * WebSocketData - Holds all relevant fields received from the WebSocket JSON payload.
 */
struct WebSocketData {
  String mode = "";
  String theme = "";
  String switchState = "";
  String pack = "";
  String safety = "";
  uint8_t wandPower = 5; // Default to max power.
  String wandMode = "";
  String firing = "";
  String cable = "";
  String cyclotron = "";
  String temperature = "";
};
WebSocketData wsData; // Instance of WebSocketData struct.

/*
 * Wand Firing Modes + Settings
 */
enum STREAM_MODES { PROTON, STASIS, SLIME, MESON, SPECTRAL, HOLIDAY_HALLOWEEN, HOLIDAY_CHRISTMAS, SPECTRAL_CUSTOM, SETTINGS, SELFTEST };
enum STREAM_MODES STREAM_MODE;
bool b_firing = false;

/*
 * Special Flags for Self-Test Mode
 */
enum STREAM_MODES STREAM_MODE_PREV;
bool b_testing = false;
