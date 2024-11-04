/**
 *   GPStar External - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023-2024 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
 *                         & Dustin Grau <dustin.grau@gmail.com>
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
 * Pin for Addressable LEDs
 */
#define DEVICE_LED_PIN 23
#define BUILT_IN_LED 2
#define DEVICE_NUM_LEDS 1
CRGB device_leds[DEVICE_NUM_LEDS];

/*
 * Pins for RGB LEDs
 */
#define LED_R_PIN 4
#define LED_G_PIN 18
#define LED_B_PIN 19

/*
 * Addressable LED Devices
 */
enum device {
  PRIMARY_LED
};

/*
 * Delay for LED blinking.
 */
millisDelay ms_blink;
const uint8_t i_blink_delay = 200;
bool b_blink = true;

/*
 * Wand Firing Modes + Settings
 */
enum POWER_LEVELS { LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, LEVEL_5 };
enum POWER_LEVELS POWER_LEVEL;
enum STREAM_MODES { PROTON, SLIME, STASIS, MESON, SPECTRAL, HOLIDAY, SPECTRAL_CUSTOM, SETTINGS };
enum STREAM_MODES STREAM_MODE;
bool b_firing = false;
uint8_t i_power = 0;

// Forward declarations.
void debug(String message);