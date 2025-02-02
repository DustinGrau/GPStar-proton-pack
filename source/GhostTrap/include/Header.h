/**
 *   GPStar Ghost Trap - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2025 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
 *                    & Dustin Grau <dustin.grau@gmail.com>
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
#define BUILT_IN_LED 21 // GPIO21 for Waveshare ESP32-S3 Mini (RGB LED)
#define DEVICE_NUM_LEDS 1
CRGB device_leds[DEVICE_NUM_LEDS];

/*
 * Pins for Devices
 */
#define BLOWER_PIN 5
#define CENTER_LED 6
#define SMOKE_PIN 7
#define DOOR_CLOSED_PIN 8
#define DOOR_OPENED_PIN 9

/*
 * Timers for Devices
 */
millisDelay ms_blower;
millisDelay ms_centerled;
millisDelay ms_smoke;

/*
 * Limits for Operation
 */
const uint8_t i_min_power = 0; // Essentially a "low" state (off).
const uint8_t i_max_power = 255; // Essentially a "high" state (on).
const uint16_t i_smoke_duration_min = 1000; // Minimum "sane" time to run smoke.
const uint16_t i_smoke_duration_max = 10000; // Do not allow smoke to run more than 10 seconds.

/*
 * Global flag to enable/disable smoke.
 */
bool b_smoke_enabled = true;

/*
 * UI Status Display Type
 */
enum DISPLAY_TYPES : uint8_t {
  STATUS_TEXT = 0,
  STATUS_GRAPHIC = 1,
  STATUS_BOTH = 2
};
enum DISPLAY_TYPES DISPLAY_TYPE;

/*
 * Device States
 */
enum DOOR_STATES : uint8_t {
  DOORS_UNKNOWN = 0,
  DOORS_CLOSED = 1,
  DOORS_OPENED = 2
};
enum DOOR_STATES DOOR_STATE;

// Forward declarations.
void debug(String message);
