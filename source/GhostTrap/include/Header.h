/**
 *   GPStar Ghost Trap - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2025 Dustin Grau <dustin.grau@gmail.com>
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

// Reserved for I2C Bus
// SDA -> GPIO21
// SLC -> GPIO22

// Reserved for Serial2
// RX2 -> GPIO16
// TX2 -> GPIO17

/*
 * Pin for Addressable LEDs
 */
//#define BUILT_IN_LED 21 // GPIO21 for Waveshare ESP32-S3 Mini (RGB LED)
#define BUILT_IN_LED 2 // GPIO2 for ESP32-S3 (Blue LED)
#define DEVICE_NUM_LEDS 1
CRGB device_leds[DEVICE_NUM_LEDS];

/*
 * Pins for Devices
 */
#define BLOWER_PIN 18
#define CENTER_LED 19
#define SMOKE_PIN 23
#define TOP_2WHITE 32 // Frutto top panel (2 white LEDs)
#define TOP_PIXELS 33 // Frutto top panel (12 RGB pixels)
#define DOOR_CLOSED_PIN 34 // Green Socket (Input Only)
#define DOOR_OPENED_PIN 35 // Red Socket (Input Only)

/*
 * Frutto Trap Top (Pixels)
 */
#define NUM_TOP_PIXELS 12
CRGB top_leds[NUM_TOP_PIXELS];

/*
 * Timers for Devices
 */
millisDelay ms_blower;
millisDelay ms_light;
millisDelay ms_smoke;

/*
 * Limits for Operation
 */
const uint8_t i_min_power = 0; // Essentially a "low" state (off).
const uint8_t i_max_power = 255; // Essentially a "high" state (on).
const uint16_t i_smoke_duration_min = 1000; // Minimum "sane" time to run smoke (1 second).
const uint16_t i_smoke_duration_max = 10000; // Do not allow smoke to run more than 10 seconds.
const uint16_t i_blower_start_delay = 1500; // Time to delay start of the blower for smoke, allowing built-up (1.5 second).

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
enum DOOR_STATES LAST_DOOR_STATE;

/*
 * Smoke Control
 */
bool b_smoke_opened_enabled = false;
bool b_smoke_closed_enabled = false;
uint16_t i_smoke_opened_duration = 2000;
uint16_t i_smoke_closed_duration = 3000;

// Forward declarations.
void debug(String message);
