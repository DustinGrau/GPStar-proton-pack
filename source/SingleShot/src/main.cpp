/**
 *   GPStar Single-Shot Blaster
 *   Copyright (C) 2024-2025 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
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

// Required for PlatformIO
#include <Arduino.h>

// Specify all #define statements for task scheduler first
// See: https://github.com/arkhipenko/TaskScheduler/tree/master/examples
#define _TASK_SCHEDULING_OPTIONS
#ifndef ESP32
  // This only works on ATMEGA; it will crash wifi on ESP32
  #define _TASK_SLEEP_ON_IDLE_RUN
#endif
#define _TASK_TIMECRITICAL

// See: https://github.com/arkhipenko/TaskScheduler/wiki/API-Documentation
#include <TaskScheduler.h>

// Set to 1 to enable built-in debug messages
#define DEBUG 0

// Debug macros
#if DEBUG == 1
  #define debug(...) Serial.print(__VA_ARGS__)
  #define debugf(...) Serial.printf(__VA_ARGS__)
  #define debugln(...) Serial.println(__VA_ARGS__)
#else
  #define debug(...)
  #define debugf(...)
  #define debugln(...)
#endif

// PROGMEM macros
#define PROGMEM_READU32(x) pgm_read_dword_near(&(x))
#define PROGMEM_READU16(x) pgm_read_word_near(&(x))
#define PROGMEM_READU8(x) pgm_read_byte_near(&(x))

// 3rd-Party Libraries
#include <CRC32.h>
#include <EEPROM.h>
#include <millisDelay.h>
#include <FastLED.h>
#include <avdweb_Switch.h>
#include <ht16k33.h>
#include <Wire.h>
#ifdef ESP32
  #include <HardwareSerial.h>
  #include <Adafruit_LIS3MDL.h>
  #include <SparkFunLSM6DS3.h>
#endif

// Local Files
#include "Configuration.h"
#include "MusicSounds.h"
#include "Header.h"
#include "Colours.h"
#include "Bargraph.h"
#include "Cyclotron.h"
#include "Audio.h"
#ifdef ESP32
  #include "PreferencesESP.h"
#else
  #include "PreferencesATMega.h"
#endif
#include "System.h"
#include "Actions.h"
#ifdef ESP32
  #include "Wireless.h"
#endif

// Forward declaration of scheduler task callback(s).
void animateTaskCallback();
void inputTaskCallback();

// Create the primary task scheduler.
Scheduler schedule;

// Create a task to handle all updates for LED/Bargraph animations.
// 33ms reflects a refresh rate equivalent to 30fps.
// 16ms reflects a refresh rate equivalent to 60fps.
Task animateTask(16, TASK_FOREVER, &animateTaskCallback);

// Create a task to check for user inputs via switches/encoders.
// Average visual reaction time to changes is 13-20ms.
Task inputsTask(14, TASK_FOREVER, &inputTaskCallback);

void setup() {
#ifdef ESP32
  // To save power, reduce CPU frequency to 160 MHz.
  setCpuFrequencyMhz(160);

  // Serial0 (UART0) is enabled by default; end() sets GPIO43 & GPIO44 to GPIO.
  Serial0.end();

  /* This loop changes GPIO39~GPIO42 to Function 1, which is GPIO.
   * PIN_FUNC_SELECT sets the IOMUX function register appropriately.
   * IO_MUX_GPIO0_REG is the register for GPIO0, which we then seek from.
   * PIN_FUNC_GPIO is a define for Function 1, which sets the pins to GPIO mode.
   */
  for(uint8_t gpio_pin = 39; gpio_pin < 43; gpio_pin++) {
    PIN_FUNC_SELECT(IO_MUX_GPIO0_REG + (gpio_pin * 4), PIN_FUNC_GPIO);
  }
#else
  Serial.begin(9600); // Standard HW serial (USB) console.
#endif

  // Setup the audio device for this controller.
  setupAudioDevice();

  // Change PWM frequency for the vibration motor, we do not want it high pitched.
#ifdef ESP32
  // Use of the register is not needed by ESP32, as it uses a different method for PWM.
#else
  // For ATmega2560, we set the PWM frequency for pin 11 (TCCR5B) to 122.55 Hz.
  TCCR1B = (TCCR1B & B11111000) | B00000100;
  pinMode(VIBRATION_PIN, OUTPUT); // Vibration motor is PWM, so fallback to default pinMode just to be safe.
#endif

#ifdef ESP32
  // Begin by setting up WiFi as a prerequisite to all else.
  if(startWiFi()) {
    // Start the local web server.
    startWebServer();

    // Begin timer for remote client events.
    ms_cleanup.start(i_websocketCleanup);
    ms_apclient.start(i_apClientCount);
    ms_otacheck.start(i_otaCheck);
  }
#endif

  // System LEDs
  FastLED.addLeds<NEOPIXEL, SYSTEM_LED_PIN>(system_leds, CYCLOTRON_LED_COUNT + BARREL_LED_COUNT).setCorrection(TypicalLEDStrip);
  FastLED.setMaxRefreshRate(0); // Disable FastLED's blocking 2.5ms delay.

  // RGB Vent Light
  FastLED.addLeds<NEOPIXEL, TOP_LED_PIN>(vent_leds, VENT_LEDS_MAX).setCorrection(TypicalLEDStrip);
  vent_leds[0] = getHueAsRGB(C_WHITE); // Set vent light array to white for initial reset.
  vent_leds[1] = getHueAsRGB(C_WHITE); // Set top light array to white for initial reset.

  // Setup default system settings.
  VIBRATION_MODE_EEPROM = VIBRATION_FIRING_ONLY;
  VIBRATION_MODE = VIBRATION_MODE_EEPROM;
  DEVICE_MENU_LEVEL = MENU_LEVEL_1;
  MENU_OPTION_LEVEL = OPTION_5;
  POWER_LEVEL = LEVEL_1;

  // Set callback events for these toggles, which need to count the activations for EEPROM menu entry.
  switch_vent.setPushedCallback(&ventSwitched); // For the LED EEPROM Menu
  switch_device.setPushedCallback(&deviceSwitched); // For the Config EEPROM Menu

  // Rotary encoder on the top of the device.
  encoder.initialize();

  // Setup the bargraph after a brief delay.
  delay(10);
  setupBargraph();

  // Initialize all non-addressable LEDs
  led_Status.initialize();
  led_SloBlo.initialize();
  led_Clippard.initialize();
#ifndef ESP32
  led_TopWhite.initialize();
  led_Vent.initialize();
#endif
  led_Hat1.initialize();
  led_Hat2.initialize();
  led_Tip.initialize();

  // Device status.
  DEVICE_STATUS = MODE_OFF;
  DEVICE_ACTION_STATUS = ACTION_IDLE;

  // We bootup the device in the classic proton mode.
  STREAM_MODE = PROTON;

  // Load any saved settings stored in the EEPROM memory of the GPStar Single-Shot Blaster.
  if(b_eeprom) {
    readEEPROM();
  }

  // Reset the master volume. Important to keep this as we startup the system at the lowest volume.
  // Then the EEPROM reads any settings if required, then we reset the volume.
  updateMasterVolume(true);

  // Start up some timers for MODE_ORIGINAL.
  ms_slo_blo_blink.start(i_slo_blo_blink_delay);

  // Starts music track completion check timer.
  ms_check_music.start(i_music_check_delay);

  // Make sure lights are off, including the bargraph.
  allLightsOff();

  // Execute the System POST (Power On Self Test)
  systemPOST();

  // Set the options for the tasks so that it "catches up" if there is a delay.
  animateTask.setSchedulingOption(TASK_SCHEDULE);
  inputsTask.setSchedulingOption(TASK_SCHEDULE);

  // Initialize the task scheduler and enable the core tasks.
  schedule.init();
  schedule.addTask(animateTask);
  schedule.addTask(inputsTask);
  animateTask.enable();
  inputsTask.enable();
}

// Task callback for handling animations.
void animateTaskCallback() {
  // Update bargraph with latest state and pattern changes.
  if(ms_firing_pulse.isRunning()) {
    // Increase the speed for updates while this timer is still running.
    bargraphUpdate(POWER_LEVEL - 1);
  }
  else {
    // Otherwise run with the standard timing.
    bargraphUpdate();
  }

  // Keep the cyclotron spinning as necessary.
  checkCyclotron();

  // Update all addressable LEDs to reflect any changes.
  FastLED[0].showLeds(255);

  // Update the vent/top LEDs.
  if(b_vent_lights_changed) {
    if(b_rgb_vent_light) {
      // Only commit an update if the addressable LED panel is installed.
      FastLED[1].showLeds(255);
    }

    b_vent_lights_changed = false;
  }
}

// Task callback for handling user inputs.
void inputTaskCallback() {
#ifdef ESP32
  webLoops(); // Handle web server loops, including WebSocket events and OTA updates.
#endif

  updateAudio(); // Update the state of the available sound board.

  checkMusic(); // Perform music control here as this is a standalone device.

  switchLoops(); // Standard polling for switch/button changes via user inputs.

  // Get the current state of any input devices (toggles, buttons, and switches).
  checkRotaryEncoder();
  checkMenuVibration();

  // Handle button press events based on current device state and menu level (for config/EEPROM purposes).
  checkDeviceAction();

  // Perform updates/actions based on timer events.
  checkGeneralTimers();
}

void loop() {
  // Task execution via the scheduler.
  schedule.execute();
}
