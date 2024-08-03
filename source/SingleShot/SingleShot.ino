/**
 *   GPStar Single-Shot Blaster
 *   Copyright (C) 2024 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
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

#if defined(__AVR_ATmega2560__)
  #define GPSTAR_NEUTRONA_DEVICE_PCB
#endif

// Set to 1 to enable built-in debug messages
#define DEBUG 1

// Debug macros
#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

// PROGMEM macro
#define PROGMEM_READU32(x) pgm_read_dword_near(&(x))
#define PROGMEM_READU16(x) pgm_read_word_near(&(x))
#define PROGMEM_READU8(x) pgm_read_byte_near(&(x))

// 3rd-Party Libraries
#include <CRC32.h>
#include <digitalWriteFast.h>
#include <EEPROM.h>
#include <millisDelay.h>
#include <FastLED.h>
#include <avdweb_Switch.h>
#include <ht16k33.h>
#include <Wire.h>

// Local Files
#include "Configuration.h"
#include "MusicSounds.h"
#include "Header.h"
#include "Bargraph.h"
#include "Colours.h"
#include "Audio.h"
#include "Preferences.h"

void setup() {
  Serial.begin(9600); // Standard serial (USB) console.

  // Setup the audio device for this controller.
  setupAudioDevice();

  // Change PWM frequency of pin 3 and 11 for the vibration motor, we do not want it high pitched.
  TCCR2B = (TCCR2B & B11111000) | (B00000110); // for PWM frequency of 122.55 Hz

  // System LEDs
  FastLED.addLeds<NEOPIXEL, SYSTEM_LED_PIN>(system_leds, CYCLOTRON_LED_COUNT + BARREL_LED_COUNT);

  // Setup default system settings.
  VIBRATION_MODE_EEPROM = VIBRATION_NONE; // VIBRATION_ALWAYS
  DEVICE_MENU_LEVEL = MENU_LEVEL_1;
  POWER_LEVEL = LEVEL_1;

  // Set callback events for these toggles, which need to count the activations for EEPROM menu entry.
  switch_vent.setPushedCallback(&ventSwitched);
  switch_device.setPushedCallback(&deviceSwitched);

  // Rotary encoder on the top of the device.
  encoder.initialize();

  // Setup the bargraph after a brief delay.
  delay(10);
  setupBargraph();

  // Initialize all non-addressable LEDs
  led_SloBlo.initialize();
  led_Clippard.initialize();
  led_TopWhite.initialize();
  led_Vent.initialize();
  led_Hat1.initialize();
  led_Hat2.initialize();
  led_Tip.initialize();

  pinMode(vibration, OUTPUT); // Vibration motor is PWM, so fallback to default pinMode just to be safe.

  // Make sure lights are off.
  allLightsOff();

  // Device status.
  DEVICE_STATUS = MODE_OFF;
  DEVICE_ACTION_STATUS = ACTION_IDLE;

  // We bootup the device in the classic proton mode.
  STREAM_MODE = PROTON;

  // Load any saved settings stored in the EEPROM memory of the GPStar Single-Shot Blaster.
  if(b_eeprom) {
    readEEPROM();
  }

  // Start the button mash check timer.
  ms_bmash.start(0);

  // Start up some timers for MODE_ORIGINAL.
  ms_slo_blo_blink.start(i_slo_blo_blink_delay);

  // Initialize the fastLED state update timer.
  ms_fast_led.start(i_fast_led_delay);

  // Check music timer for bench test mode only.
  ms_check_music.start(i_music_check_delay);

  // Reset our master volume manually.
  resetMasterVolume();

  // System Power On Self Test
  systemPOST();
}

void loop() {
  updateAudio(); // Update the state of the selected sound board.

  checkMusic(); // Music control is here since in standalone mode.

  mainLoop(); // Continue on to the main loop.
}

void systemPOST() {
  uint8_t i_delay = 100;

  // Play a sound to test the audio system.
  playEffect(S_DEVICE_READY);

  // Turn on all bargraph elements and force an update
  bargraph.reset();
  bargraph.full();
  bargraph.commit();

  // These go HIGH to turn on.
  led_SloBlo.turnOn();
  delay(i_delay);
  led_Clippard.turnOn();
  delay(i_delay);
  led_Hat2.turnOn();
  delay(i_delay);

  // These go LOW to turn on.
  led_Vent.turnOn();
  delay(i_delay);
  led_TopWhite.turnOn();
  delay(i_delay);

  led_Tip.turnOn();
  delay(i_delay);

  // Sequentially turn on all LEDs in the cyclotron.
  for(uint8_t i = 0; i < i_num_cyclotron_leds; i++) {
    system_leds[i] = getHueAsRGB(C_RED);
    FastLED.show();
    delay(i_delay);
  }

  // Turn on the front barrel.
  system_leds[i_barrel_led] = getHueAsRGB(C_WHITE);
  FastLED.show();

  delay(i_delay * 8);

  allLightsOff(); // Turn off all lights, including the bargraph.

  // Turn the bargraph off and force a state change.
  bargraph.off();
  bargraph.commit();
}

// Return current power level as a number (eg: 1-5)
uint8_t getPowerLevel() {
  switch(POWER_LEVEL){
    case LEVEL_1:
    default:
      return 1;
    break;
    case LEVEL_2:
      return 2;
    break;
    case LEVEL_3:
      return 3;
    break;
    case LEVEL_4:
      return 4;
    break;
    case LEVEL_5:
      return 5;
    break;
  }
}

bool increasePowerLevel() {
  switch(POWER_LEVEL){
    case LEVEL_1:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_2;
    break;
    case LEVEL_2:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_3;
    break;
    case LEVEL_3:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_4;
    break;
    case LEVEL_4:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_5;
    break;
    case LEVEL_5:
      // No-op, at highest level.
    break;
  }

  // Returns true if value was changed.
  return (POWER_LEVEL_PREV != POWER_LEVEL);
}

bool decreasePowerLevel() {
  switch(POWER_LEVEL){
    case LEVEL_1:
      // No-op, at lowest level.
    break;
    case LEVEL_2:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_1;
    break;
    case LEVEL_3:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_2;
    break;
    case LEVEL_4:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_3;
    break;
    case LEVEL_5:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_4;
    break;
  }

  // Returns true if value was changed.
  return (POWER_LEVEL_PREV != POWER_LEVEL);
}

void mainLoop() {
  // Get the current state of any input devices (toggles, buttons, and switches).
  switchLoops();
  checkSwitches();
  checkRotaryEncoder();
  checkMenuVibration();

  if(DEVICE_ACTION_STATUS != ACTION_FIRING) {
    if(ms_bmash.remaining() < 1) {
      // Clear counter until user begins firing (post any lock-out period).
      i_bmash_count = 0;

      if(b_device_mash_error) {
        // Return the device to a normal firing state after lock-out from button mashing.
        b_device_mash_error = false;

        DEVICE_STATUS = MODE_ON;
        DEVICE_ACTION_STATUS = ACTION_IDLE;

        postActivation();

        // stopEffect(S_SMASH_ERROR_LOOP);
        // playEffect(S_SMASH_ERROR_RESTART);

        bargraph.clear();
      }
    }
  }

  switch(DEVICE_STATUS) {
    case MODE_OFF:
      if(DEVICE_ACTION_STATUS != ACTION_CONFIG_EEPROM_MENU) {
        if(switch_grip.pushed()) {
          if(DEVICE_ACTION_STATUS != ACTION_SETTINGS) {
            playEffect(S_CLICK);

            DEVICE_ACTION_STATUS = ACTION_SETTINGS;
            DEVICE_MENU_LEVEL = MENU_LEVEL_1;

            i_device_menu = 5;
            ms_settings_blinking.start(i_settings_blinking_delay);

            bargraph.clear();

            // Make sure some of the device lights are off.
            allLightsOffMenuSystem();
          }
          else {
            // Only exit the settings menu when on menu #5 in the top menu
            if(i_device_menu == 5 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && DEVICE_ACTION_STATUS == ACTION_SETTINGS) {
              deviceExitMenu();
            }
          }
        }
      }

      // Reset the count of the device switch
      if(!switch_intensify.on()) {
        deviceSwitchedCount = 0;
        ventSwitchedCount = 0;
      }

      if(DEVICE_ACTION_STATUS != ACTION_SETTINGS && DEVICE_ACTION_STATUS != ACTION_CONFIG_EEPROM_MENU
         && switch_intensify.on() && ventSwitchedCount >= 5) {
        stopEffect(S_BEEPS);
        playEffect(S_BEEPS);

        stopEffect(S_VOICE_EEPROM_CONFIG_MENU);
        playEffect(S_VOICE_EEPROM_CONFIG_MENU);

        i_device_menu = 5;

        DEVICE_ACTION_STATUS = ACTION_CONFIG_EEPROM_MENU;
        DEVICE_MENU_LEVEL = MENU_LEVEL_1;

        ms_settings_blinking.start(i_settings_blinking_delay);

        // Make sure some of the device lights are off.
        allLightsOffMenuSystem();
      }

      // If the power indicator is enabled. Blink the LED on the Single-Shot Blaster body next to the clippard valve to indicator the system has battery power.
      if(b_power_on_indicator && DEVICE_ACTION_STATUS == ACTION_IDLE) {
        if(ms_power_indicator.isRunning() && ms_power_indicator.remaining() < 1) {
          if(!ms_power_indicator_blink.isRunning() || ms_power_indicator_blink.justFinished()) {
            ms_power_indicator_blink.start(i_ms_power_indicator_blink);
          }

          if(ms_power_indicator_blink.remaining() < i_ms_power_indicator_blink / 2) {
            led_Clippard.turnOff();
          }
          else {
            led_Clippard.turnOn();
          }
        }
      }
    break;

    case MODE_ERROR:
      if(ms_hat_2.remaining() < i_hat_2_delay / 2) {
        led_Clippard.turnOff();
        led_SloBlo.turnOff();
        led_TopWhite.turnOff();
        led_Hat2.turnOff();
      }
      else {
        led_Clippard.turnOn();
        led_SloBlo.turnOn();
        led_TopWhite.turnOn();
        led_Hat2.turnOn();
      }

      if(ms_hat_2.justFinished()) {
        ms_hat_2.start(i_hat_2_delay);

        if(!b_device_mash_error) {
          playEffect(S_BEEPS_LOW);
          playEffect(S_BEEPS);
        }
      }

      if(ms_hat_1.justFinished()) {
        if(!b_device_mash_error) {
          playEffect(S_BEEPS);
        }

        ms_hat_1.start(i_hat_2_delay * 4);
      }
    break;

    case MODE_ON:
      if(!ms_hat_1.isRunning() && !ms_hat_2.isRunning()) {
        // Hat 2 stays solid while the Single-Shot Blaster is on.
        led_Hat2.turnOn();
      }

      // Top white light.
      if(ms_white_light.justFinished()) {
        ms_white_light.repeat();
        if(led_TopWhite.getState() == LOW) {
          led_TopWhite.turnOff();
        }
        else {
          led_TopWhite.turnOn();
        }
      }

      vibrationSetting();
    break;
  }

  // Handle button press events based on current device state and menu level (for config/EEPROM purposes).
  checkDeviceAction();

  if(b_firing && DEVICE_ACTION_STATUS != ACTION_FIRING) {
    modeFireStop();
  }

  // Play the firing pulse effect animation.
  if(ms_firing_pulse.justFinished()) {
    firePulseEffect();
  }

  // Update the barrel LEDs and restart the timer.
  if(ms_fast_led.justFinished()) {
    FastLED.show();
    ms_fast_led.start(i_fast_led_delay);
  }

  bargraphUpdate(1); // Update bargraph with latest state changes.
}

// Manage lights in pairs to move in a predefined sequence, fading each light in and out.
void updateCyclotron(uint8_t i_colour) {
  static bool sb_toggle = true; // Static toggle to remain scoped to this function between calls
  static uint8_t sb_pairing = 0; // Which pair of LEDs to use for each "cycle" of fade in/out actions
  static uint8_t si_brightness_in = i_cyclotron_min_brightness; // Static brightness variable for fade-in effect
  static uint8_t si_brightness_out = i_cyclotron_max_brightness; // Static brightness variable for fade-out effect

  if(ms_cyclotron.justFinished()) {
    // Change the timing (delay) based on the power level selected.
    uint16_t i_dynamic_delay = i_base_cyclotron_delay - ((getPowerLevel() - 1) * (i_base_cyclotron_delay - i_min_cyclotron_delay) / 4);
    ms_cyclotron.start(i_dynamic_delay);

    // Increment brightness for fade-in effect
    if(si_brightness_in < i_cyclotron_max_brightness - 1) {
      si_brightness_in += i_cyc_fade_step;

      if(si_brightness_in > i_cyclotron_max_brightness - 1) {
        si_brightness_in = i_cyclotron_max_brightness;
      }
    }

    // Decrement brightness for fade-out effect
    if(si_brightness_out > i_cyclotron_min_brightness + 1) {
      si_brightness_out -= i_cyc_fade_step;

      if(si_brightness_out < i_cyclotron_min_brightness + 1) {
        si_brightness_out = i_cyclotron_min_brightness;
      }
    }

    // Toggle between the LEDs in the i_cyclotron_pair using the given color.
    if(sb_toggle) {
      system_leds[i_cyclotron_pair[sb_pairing][0]] = getHueAsRGB(i_colour).nscale8(si_brightness_in);  // Fade in LED 1 in the pair
      system_leds[i_cyclotron_pair[sb_pairing][1]] = getHueAsRGB(i_colour).nscale8(si_brightness_out); // Fade out LED 2 in the pair
    }
    else {
      system_leds[i_cyclotron_pair[sb_pairing][0]] = getHueAsRGB(i_colour).nscale8(si_brightness_out); // Fade out LED 1 in the pair
      system_leds[i_cyclotron_pair[sb_pairing][1]] = getHueAsRGB(i_colour).nscale8(si_brightness_in);  // Fade in LED 2 in the pair
    }

    // Toggle state and reset brightness variables after fade-in is complete.
    if (si_brightness_in == i_cyclotron_max_brightness && si_brightness_out == i_cyclotron_min_brightness) {
      sb_toggle = !sb_toggle;
      si_brightness_in = i_cyclotron_min_brightness;
      si_brightness_out = i_cyclotron_max_brightness;
      sb_pairing = (sb_pairing + 1) % i_cyclotron_max_steps; // Change to next pair on each toggle.
    }
  }
}

void deviceTipSpark() {
  i_heatup_counter = 0;
  i_heatdown_counter = 100;
  i_bmash_spark_index = 0;
  ms_device_heatup_fade.start(i_delay_heatup);
}

// Change the DEVICE_STATE here based on switches changing or pressed.
void checkSwitches() {
  if(ms_slo_blo_blink.justFinished()) {
    ms_slo_blo_blink.start(i_slo_blo_blink_delay);
  }

  switch(DEVICE_STATUS) {
    case MODE_OFF:
      if(switch_activate.on() && DEVICE_ACTION_STATUS == ACTION_IDLE) {
        // Turn device on.
        DEVICE_ACTION_STATUS = ACTION_ACTIVATE;
      }
    break;

    case MODE_ERROR:
      if(!switch_activate.on()) {
        b_device_boot_error_on = false;
        deviceOff();
      }
    break;

    case MODE_ON:
      gripButtonCheck();

      // Determine the light status on the device and any beeps.
      deviceLightControlCheck();

      // Check if we should fire, or if the device was turned off.
      fireControlCheck();
    break;
  }
}

// Determine the light status on the device and any beeps.
void deviceLightControlCheck() {
  // Vent light and first stage of the safety system.
  if(switch_vent.on()) {
    if(b_vent_light_control) {
      // Vent light on, brightness dependent on mode.
      if(DEVICE_ACTION_STATUS == ACTION_FIRING || (ms_semi_automatic_firing.isRunning() && !ms_semi_automatic_firing.justFinished())) {
        led_Vent.dim(0); // 0 = Full Power
      }
      else {
        // Adjust brightness based on the power level.
        switch(POWER_LEVEL) {
          case LEVEL_1:
          default:
            led_Vent.dim(220);
          break;
          case LEVEL_2:
            led_Vent.dim(190);
          break;
          case LEVEL_3:
            led_Vent.dim(160);
          break;
          case LEVEL_4:
            led_Vent.dim(130);
          break;
          case LEVEL_5:
            led_Vent.dim(100);
          break;
        }
      }
    }
    else {
      led_Vent.turnOn();
    }
  }
  else if(!switch_vent.on()) {
    // Vent light and top white light off.
    led_Vent.turnOff();
  }
}

void deviceOff() {
  if(b_device_boot_error_on) {
    stopEffect(S_BEEPS_LOW);
    stopEffect(S_BEEPS);
    stopEffect(S_BEEPS);
  }

  stopEffect(S_BOOTUP);
  //stopEffect(S_SMASH_ERROR_RESTART);

  b_sound_afterlife_idle_2_fade = true;

  if(DEVICE_ACTION_STATUS == ACTION_ERROR && !b_device_boot_error_on && !b_device_mash_error) {
    // We are exiting Device Boot Error, so change device state back to off/idle.
    DEVICE_STATUS = MODE_OFF;
    DEVICE_ACTION_STATUS = ACTION_IDLE;
  }
  else if(DEVICE_ACTION_STATUS != ACTION_ERROR && (b_device_boot_error_on || b_device_mash_error)) {
    // We are entering either Device Boot Error mode or Button Mash Timeout mode, so do nothing.
  }
  else {
    // Full device shutdown in all other situations.
    DEVICE_STATUS = MODE_OFF;
    DEVICE_ACTION_STATUS = ACTION_IDLE;

    if(b_device_mash_error) {
      // stopEffect(S_SMASH_ERROR_LOOP);
      // stopEffect(S_SMASH_ERROR_RESTART);
    }

    // Turn off any barrel spark effects and reset the button mash lockout.
    if(b_device_mash_error) {
      barrelLightsOff();
      b_device_mash_error = false;
    }

    stopEffect(S_SHUTDOWN);
    playEffect(S_SHUTDOWN);
  }

  soundIdleLoopStop();

  vibrationOff();

  // Stop firing if the device is turned off.
  if(b_firing) {
    modeFireStop();
  }

  if(DEVICE_ACTION_STATUS != ACTION_ERROR && b_device_mash_error) {
    // playEffect(S_DEVICE_MASH_ERROR);
  }

  // Clear counter until user begins firing again.
  i_bmash_count = 0;

  // Turn off some timers.
  ms_cyclotron.stop();
  ms_settings_blinking.stop();
  ms_semi_automatic_check.stop();
  ms_semi_automatic_firing.stop();
  ms_hat_1.stop();
  ms_hat_2.stop();

  switch(DEVICE_STATUS) {
    case MODE_OFF:
      // Turn off all device lights.
      allLightsOff();

      // Start the timer for the power on indicator option.
      if(b_power_on_indicator) {
        ms_power_indicator.start(i_ms_power_indicator);
      }

      deviceSwitchedCount = 0;
      ventSwitchedCount = 0;
    break;
    default:
      // Do nothing if we aren't MODE_OFF
    break;
  }
}

// Called from checkSwitches(); Check if we should fire, or if the device was turned off.
void fireControlCheck() {
  // Firing action stuff and shutting cyclotron and the Single-Shot Blaster off.
  if(DEVICE_ACTION_STATUS != ACTION_SETTINGS) {
    // If Activate switch is down, turn device off.
    if(!switch_activate.on()) {
      DEVICE_ACTION_STATUS = ACTION_OFF;
      return;
    }

    if(i_bmash_count >= i_bmash_max) {
      // User has exceeded "normal" firing rate.
      switch(STREAM_MODE) {
        case PROTON:
        default:
          stopEffect(S_FIRE_BLAST);
        break;
      }

      b_device_mash_error = true;
      modeError();
      deviceTipSpark();

      // Adjust the cool down lockout period based on the power level.
      switch(POWER_LEVEL) {
        case LEVEL_1:
        default:
          ms_bmash.start(i_bmash_cool_down);
        break;
        case LEVEL_2:
          ms_bmash.start(i_bmash_cool_down + 500);
        break;
        case LEVEL_3:
          ms_bmash.start(i_bmash_cool_down + 1000);
        break;
        case LEVEL_4:
          ms_bmash.start(i_bmash_cool_down + 1500);
        break;
        case LEVEL_5:
          ms_bmash.start(i_bmash_cool_down + 2000);
        break;
      }
    }
    else {
      if(switch_intensify.on() && switch_device.on() && switch_vent.on()) {
        switch(STREAM_MODE) {
          case PROTON:
          default:
            if(DEVICE_ACTION_STATUS != ACTION_FIRING) {
              DEVICE_ACTION_STATUS = ACTION_FIRING;
            }

            if(ms_bmash.remaining() < 1) {
              // Clear counter/timer until user begins firing.
              i_bmash_count = 0;
              ms_bmash.start(i_bmash_delay);
            }

            if(!b_firing_intensify) {
              // Increase count each time the user presses a firing button.
              i_bmash_count++;
            }

            b_firing_intensify = true;
          break;
        }
      }

      if(STREAM_MODE == PROTON && DEVICE_ACTION_STATUS == ACTION_FIRING) {
        if(switch_grip.on()) {
          b_firing_alt = true;
        }
      }
      else if(switch_grip.on() && switch_device.on() && switch_vent.on()) {
        switch(STREAM_MODE) {
          case PROTON:
            // Handle Primary Blast fire start here.
            if(!b_firing_semi_automatic && ms_semi_automatic_check.remaining() < 1) {
              // Start rate-of-fire timer.
              ms_semi_automatic_check.start(i_single_shot_rate);

              modePulseStart();

              b_firing_semi_automatic = true;
            }
          break;

          default:
            // Do nothing.
          break;
        }
      }

      if(!switch_intensify.on()) {
        switch(STREAM_MODE) {
          case PROTON:
          default:
            if(b_firing && b_firing_intensify) {
              if(!b_firing_alt) {
                DEVICE_ACTION_STATUS = ACTION_IDLE;
              }

              b_firing_intensify = false;
            }
          break;
        }
      }

      if(!switch_grip.on()) {
        switch(STREAM_MODE) {
          case PROTON:
            // Handle resetting semi-auto bool here.
            b_firing_semi_automatic = false;
          break;

          default:
            // Do nothing.
          break;
        }
      }
    }
  }
  else if(DEVICE_ACTION_STATUS == ACTION_SETTINGS) {
    // If Activate switch is down, turn device off.
    if(!switch_activate.on()) {
      DEVICE_ACTION_STATUS = ACTION_OFF;
      return;
    }

    if(DEVICE_ACTION_STATUS == ACTION_IDLE) {
      // Play a little spark effect if the user tries to fire while the ribbon cable is removed.
      if((switch_intensify.pushed() || (switch_grip.pushed())) && !ms_device_heatup_fade.isRunning() && switch_vent.on() && switch_device.on()) {
        // stopEffect(S_DEVICE_MASH_ERROR);
        // playEffect(S_DEVICE_MASH_ERROR);
        deviceTipSpark();
      }
    }
  }
}

// Called from checkSwitches(); Used to enter the settings menu in MODE_SUPER_HERO.
void gripButtonCheck() {
  if(DEVICE_ACTION_STATUS != ACTION_FIRING && DEVICE_ACTION_STATUS != ACTION_OFF) {
    if((!switch_device.on() || !switch_vent.on()) && switch_grip.pushed()) {
      // Only exit the settings menu when on menu #5.
      if(i_device_menu == 5) {
        // Switch between firing mode and settings mode.
        if(DEVICE_ACTION_STATUS != ACTION_SETTINGS) {
          DEVICE_ACTION_STATUS = ACTION_SETTINGS;

          ms_settings_blinking.start(i_settings_blinking_delay);

          // Clear the 28 segment bargraph.
          bargraph.clear();
        }
        else {
          DEVICE_ACTION_STATUS = ACTION_IDLE;
          ms_settings_blinking.stop();
          bargraph.clear();
        }

        playEffect(S_CLICK);
      }
    }
    else if(DEVICE_ACTION_STATUS == ACTION_SETTINGS && switch_vent.on() && switch_device.on()) {
      // Exit the settings menu if the user turns the device switch back on.
      DEVICE_ACTION_STATUS = ACTION_IDLE;
      ms_settings_blinking.stop();
      bargraph.clear();
    }
  }
}

void modeError() {
  deviceOff();

  DEVICE_STATUS = MODE_ERROR;
  DEVICE_ACTION_STATUS = ACTION_ERROR;

  if(!b_device_mash_error) {
    // This is used for controlling the bargraph beeping while in boot error mode.
    ms_hat_1.start(i_hat_2_delay * 4);
    ms_hat_2.start(i_hat_2_delay);
    ms_settings_blinking.start(i_settings_blinking_delay);

    playEffect(S_BEEPS_LOW);
    playEffect(S_BEEPS);
    playEffect(S_BEEPS);
  }
  else if(b_device_mash_error) {
    // playEffect(S_SMASH_ERROR_LOOP, true, i_volume_effects, true, 2500);
  }
}

void modeActivate() {
  b_sound_afterlife_idle_2_fade = true;

  // The device was started while the top switch was already on, so let's put the device into startup error mode.
  if(switch_device.on() && b_device_boot_errors) {
    b_device_boot_error_on = true;
    modeError();
  }
  else {
    // Device is officially activated and on.
    DEVICE_STATUS = MODE_ON;

    // Proper startup. Continue booting up the device.
    DEVICE_ACTION_STATUS = ACTION_IDLE;

    // Clear counter until user begins firing.
    i_bmash_count = 0;
  }

  b_device_mash_error = false;

  postActivation(); // Enable lights and bargraph after device activation.
}

void postActivation() {
  if(DEVICE_STATUS != MODE_ERROR) {
    if(switch_vent.on()) {
      b_all_switch_activation = true; // If vent switch is already on when Activate is flipped, set to true for soundIdleLoop() to use
    }

    // Turn on slo-blo light.
    led_SloBlo.turnOn();

    // Turn on the Clippard LED.
    led_Clippard.turnOn();

    // Top white light.
    ms_white_light.start(i_top_blink_interval);
    led_TopWhite.turnOn();

    // Reset the hat light timers.
    ms_hat_1.stop();
    ms_hat_2.stop();

    stopEffect(S_BOOTUP);
    playEffect(S_BOOTUP);

    soundIdleLoop(true);

    if(bargraph.STATE == BG_OFF) {
      bargraph.reset(); // Enable bargraph for use (resets variables and turns it on).
      bargraph.PATTERN = BG_POWER_RAMP; // Bargraph idling loop.
    }
  }
}

void soundIdleLoop(bool fadeIn) {
  switch(POWER_LEVEL) {
    case LEVEL_1:
    default:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;
    case LEVEL_2:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;
    case LEVEL_3:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;
    case LEVEL_4:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;
    case LEVEL_5:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;
  }
}

void soundIdleLoopStop() {
  switch(POWER_LEVEL) {
    case LEVEL_1:
    default:
      stopEffect(S_IDLE_LOOP);
    break;
    case LEVEL_2:
      stopEffect(S_IDLE_LOOP);
    break;
    case LEVEL_3:
      stopEffect(S_IDLE_LOOP);
    break;
    case LEVEL_4:
      stopEffect(S_IDLE_LOOP);
    break;
    case LEVEL_5:
      stopEffect(S_IDLE_LOOP);
    break;
  }
}

void modePulseStart() {
  // Handles all "pulsed" fire modes.
  i_fast_led_delay = FAST_LED_UPDATE_MS;
  barrelLightsOff();

  playEffect(S_FIRE_BLAST, false, i_volume_effects, false, 0, false);

  ms_firing_pulse.start(i_firing_pulse);
  ms_semi_automatic_firing.start(350);
}

void modeFireStart() {
  i_fast_led_delay = FAST_LED_UPDATE_MS;

  //modeFireStartSounds();

  // Just in case a semi-auto was fired before we started firing a stream, stop its timer.
  ms_semi_automatic_firing.stop();

  // Turn on hat light 1.
  led_Hat1.turnOn();

  barrelLightsOff();

  ms_firing_lights.start(0);

  ms_firing_length_timer.start(i_firing_timer_length);
}

void modeFireStopSounds() {
  // Reset some sound triggers.
  b_sound_firing_intensify_trigger = false;
  b_sound_firing_alt_trigger = false;
  b_sound_firing_cross_the_streams = false;

  ms_single_blast.stop();
}

void modeFireStop() {
  DEVICE_ACTION_STATUS = ACTION_IDLE;

  b_firing = false;
  b_firing_intensify = false;
  b_firing_alt = false;

  ms_firing_lights.stop();
  ms_impact.stop();
  ms_firing_effect_end.start(0);

  led_Hat2.turnOn(); // Make sure we turn on hat light 2 in case it's off as well.

  led_Tip.turnOff();

  ms_hat_1.stop();

  modeFireStopSounds();
}

void modeFiring() {
  // Sound trigger flags.
  if(b_firing_intensify && !b_sound_firing_intensify_trigger) {
    b_sound_firing_intensify_trigger = true;
  }

  if(!b_firing_intensify && b_sound_firing_intensify_trigger) {
    b_sound_firing_intensify_trigger = false;
  }

  if(b_firing_alt && !b_sound_firing_alt_trigger) {
    b_sound_firing_alt_trigger = true;
  }

  if(!b_firing_alt && b_sound_firing_alt_trigger) {
    b_sound_firing_alt_trigger = false;
  }
}

void firePulseEffect() {
  if(i_pulse_step == 0) {
    // Force clear and reset of bargraph state.
    bargraph.clear();
    bargraph.reset();
    bargraph.commit();

    // Change bargraph animation when pulse begins.
    bargraph.PATTERN = BG_OUTER_INNER;
  }

  // Primary Blast.
  switch(i_pulse_step) {
    case 0:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED);
    break;
    case 1:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED3);
    break;
    case 2:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED5);
    break;
    case 3:
      system_leds[i_barrel_led] = getHueAsRGB(C_WHITE);
    break;
    case 4:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED4);
    break;
    case 5:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED2);
    break;
    case 6:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED);
      // Bolt has reached barrel tip, so turn on tip light.
      led_Tip.turnOn();
    break;
    case 7:
      system_leds[i_barrel_led] = getHueAsRGB(C_BLACK);
    break;
    default:
      // Do nothing if we somehow end up here.
    break;
  }

  i_pulse_step++;

  if(i_pulse_step < i_pulse_step_max) {
    ms_firing_pulse.start(i_firing_pulse);
  }
  else {
    // Animation has concluded, so reset our timer and variable.
    led_Tip.turnOff();
    ms_firing_pulse.stop();
    i_pulse_step = 0;

    // Clear the bargraph and return to ramping.
    bargraph.clear();
    bargraph.PATTERN = BG_POWER_RAMP;
  }
}

void barrelLightsOff() {
  ms_firing_pulse.stop();
  ms_firing_lights.stop();
  ms_firing_stream_effects.stop();
  ms_firing_effect_end.stop();
  ms_firing_lights_end.stop();
  ms_device_heatup_fade.stop();

  i_pulse_step = 0;
  i_heatup_counter = 0;
  i_heatdown_counter = 100;

  // Turn off the barrel LED.
  system_leds[i_barrel_led] = getHueAsRGB(C_BLACK);

  // Turn off the device barrel tip LED.
  led_Tip.turnOff();
}

void allLightsOff() {
  bargraph.clear();

  // These go LOW to turn off.
  led_SloBlo.turnOff();
  led_Clippard.turnOff(); // Turn off the front left LED under the Clippard valve.
  led_Hat1.turnOff(); // Turn off hat light 1 (not used, but just make sure).
  led_Hat2.turnOff(); // Turn off hat light 2.

  // These go HIGH to turn off.
  led_Vent.turnOff();
  led_TopWhite.turnOff();

  led_Tip.turnOff(); // Not used normally, but make sure it's off.

  // Clear all addressable LEDs by filling the array with black.
  fill_solid(system_leds, CYCLOTRON_LED_COUNT + BARREL_LED_COUNT, CRGB::Black);

  if(b_power_on_indicator && !ms_power_indicator.isRunning()) {
    ms_power_indicator.start(i_ms_power_indicator);
  }
}

void allLightsOffMenuSystem() {
  // Make sure some of the device lights are off, specifically for the Menu systems.
  led_SloBlo.turnOff();
  led_Vent.turnOff();
  led_TopWhite.turnOff();
  led_Clippard.turnOff();

  if(b_power_on_indicator) {
    ms_power_indicator.stop();
    ms_power_indicator_blink.stop();
  }
}

// Top rotary dial on the device.
void checkRotaryEncoder() {
  encoder.check(); // Update the latest state of the device resulting from any user input.

  if(encoder.STATE == ENCODER_IDLE) {
    return; // Leave when no change has occurred.
  }

  switch(DEVICE_ACTION_STATUS) {
    case ACTION_CONFIG_EEPROM_MENU:
      // Counter clockwise.
      if(encoder.STATE == ENCODER_CCW) {
        if(DEVICE_MENU_LEVEL == MENU_LEVEL_3 && i_device_menu == 5 && switch_intensify.on() && !switch_grip.on()) {
          // Adjust the volume manually
          decreaseVolumeEEPROM();
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 5 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 4 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 3 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 2 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 1 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 5 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 4 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 3 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 2 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 1 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(i_device_menu - 1 < 1) {
          switch(DEVICE_MENU_LEVEL) {
            case MENU_LEVEL_1:
              DEVICE_MENU_LEVEL = MENU_LEVEL_2;
              i_device_menu = 5;

              // Turn on some lights to visually indicate which menu we are in.
              led_SloBlo.turnOn(); // Level 2

              // Turn off the other lights.
              led_Vent.turnOff(); // Level 3
              led_TopWhite.turnOff(); // Level 4
              led_Clippard.turnOff(); // Level 5

              // Play an indication beep to notify we have changed menu levels.
              stopEffect(S_BEEPS);
              playEffect(S_BEEPS);

              stopEffect(S_VOICE_LEVEL_1);
              stopEffect(S_VOICE_LEVEL_2);
              stopEffect(S_VOICE_LEVEL_3);
              stopEffect(S_VOICE_LEVEL_4);
              stopEffect(S_VOICE_LEVEL_5);

              playEffect(S_VOICE_LEVEL_2);
            break;

            case MENU_LEVEL_2:
              DEVICE_MENU_LEVEL = MENU_LEVEL_3;
              i_device_menu = 5;

              // Turn on some lights to visually indicate which menu we are in.
              led_SloBlo.turnOn(); // Level 2
              led_Vent.turnOn(); // Level 3

              // Turn off the other lights.
              led_TopWhite.turnOff(); // Level 4
              led_Clippard.turnOff(); // Level 5

              // Play an indication beep to notify we have changed menu levels.
              stopEffect(S_BEEPS);
              playEffect(S_BEEPS);

              stopEffect(S_VOICE_LEVEL_1);
              stopEffect(S_VOICE_LEVEL_2);
              stopEffect(S_VOICE_LEVEL_3);
              stopEffect(S_VOICE_LEVEL_4);
              stopEffect(S_VOICE_LEVEL_5);

              playEffect(S_VOICE_LEVEL_3);
            break;

            case MENU_LEVEL_3:
              DEVICE_MENU_LEVEL = MENU_LEVEL_4;
              i_device_menu = 5;

              // Turn on some lights to visually indicate which menu we are in.
              led_SloBlo.turnOn(); // Level 2
              led_Vent.turnOn(); // Level 3
              led_TopWhite.turnOn(); // Level 4

              // Turn off the other lights.
              led_Clippard.turnOff(); // Level 5

              // Play an indication beep to notify we have changed menu levels.
              stopEffect(S_BEEPS);
              playEffect(S_BEEPS);

              stopEffect(S_VOICE_LEVEL_1);
              stopEffect(S_VOICE_LEVEL_2);
              stopEffect(S_VOICE_LEVEL_3);
              stopEffect(S_VOICE_LEVEL_4);
              stopEffect(S_VOICE_LEVEL_5);

              playEffect(S_VOICE_LEVEL_4);
            break;

            case MENU_LEVEL_4:
              DEVICE_MENU_LEVEL = MENU_LEVEL_5;
              i_device_menu = 5;

              // Turn on some lights to visually indicate which menu we are in.
              led_SloBlo.turnOn(); // Level 2
              led_Vent.turnOn(); // Level 3
              led_TopWhite.turnOn(); // Level 4
              led_Clippard.turnOn(); // Level 5

              // Play an indication beep to notify we have changed menu levels.
              stopEffect(S_BEEPS);
              playEffect(S_BEEPS);

              stopEffect(S_VOICE_LEVEL_1);
              stopEffect(S_VOICE_LEVEL_2);
              stopEffect(S_VOICE_LEVEL_3);
              stopEffect(S_VOICE_LEVEL_4);
              stopEffect(S_VOICE_LEVEL_5);

              playEffect(S_VOICE_LEVEL_5);
            break;

            // Menu 5 the deepest level.
            case MENU_LEVEL_5:
            default:
              i_device_menu = 1;
            break;
          }
        }
        else {
          i_device_menu--;
        }
      }

      // Clockwise.
      if(encoder.STATE == ENCODER_CW) {
        if(DEVICE_MENU_LEVEL == MENU_LEVEL_3 && i_device_menu == 5 && switch_intensify.on() && !switch_grip.on()) {
          // Adjust the volume manually
          increaseVolumeEEPROM();
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 5 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 4 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 3 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 2 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 1 && switch_intensify.on() && !switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 5 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 4 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 3 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 2 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && i_device_menu == 1 && !switch_intensify.on() && switch_grip.on()) {
        }
        else if(i_device_menu + 1 > 5) {
          switch(DEVICE_MENU_LEVEL) {
            case MENU_LEVEL_5:
              DEVICE_MENU_LEVEL = MENU_LEVEL_4;
              i_device_menu = 1;

              // Turn on some lights to visually indicate which menu we are in.
              led_SloBlo.turnOn(); // Level 2
              led_Vent.turnOn(); // Level 3
              led_TopWhite.turnOn(); // Level 4

              // Turn off the other lights.
              led_Clippard.turnOff(); // Level 5

              // Play an indication beep to notify we have changed menu levels.
              stopEffect(S_BEEPS);
              playEffect(S_BEEPS);

              stopEffect(S_VOICE_LEVEL_1);
              stopEffect(S_VOICE_LEVEL_2);
              stopEffect(S_VOICE_LEVEL_3);
              stopEffect(S_VOICE_LEVEL_4);
              stopEffect(S_VOICE_LEVEL_5);

              playEffect(S_VOICE_LEVEL_4);
            break;

            case MENU_LEVEL_4:
              DEVICE_MENU_LEVEL = MENU_LEVEL_3;
              i_device_menu = 1;

              // Turn on some lights to visually indicate which menu we are in.
              led_SloBlo.turnOn(); // Level 2
              led_Vent.turnOn(); // Level 3

              // Turn off the other lights.
              led_TopWhite.turnOff(); // Level 4
              led_Clippard.turnOff(); // Level 5

              // Play an indication beep to notify we have changed menu levels.
              stopEffect(S_BEEPS);
              playEffect(S_BEEPS);

              stopEffect(S_VOICE_LEVEL_1);
              stopEffect(S_VOICE_LEVEL_2);
              stopEffect(S_VOICE_LEVEL_3);
              stopEffect(S_VOICE_LEVEL_4);
              stopEffect(S_VOICE_LEVEL_5);

              playEffect(S_VOICE_LEVEL_3);
            break;

            case MENU_LEVEL_3:
              DEVICE_MENU_LEVEL = MENU_LEVEL_2;
              i_device_menu = 1;

              // Turn on some lights to visually indicate which menu we are in.
              led_SloBlo.turnOn(); // Level 2

              // Turn off the other lights.
              led_Vent.turnOff(); // Level 3
              led_TopWhite.turnOff(); // Level 4
              led_Clippard.turnOff(); // Level 5

              // Play an indication beep to notify we have changed menu levels.
              stopEffect(S_BEEPS);
              playEffect(S_BEEPS);

              stopEffect(S_VOICE_LEVEL_1);
              stopEffect(S_VOICE_LEVEL_2);
              stopEffect(S_VOICE_LEVEL_3);
              stopEffect(S_VOICE_LEVEL_4);
              stopEffect(S_VOICE_LEVEL_5);

              playEffect(S_VOICE_LEVEL_2);
            break;

            case MENU_LEVEL_2:
              DEVICE_MENU_LEVEL = MENU_LEVEL_1;
              i_device_menu = 1;

              // Turn off the other lights.
              led_SloBlo.turnOff(); // Level 2
              led_Vent.turnOff(); // Level 3
              led_TopWhite.turnOff(); // Level 4
              led_Clippard.turnOff(); // Level 5

              // Play an indication beep to notify we have changed menu levels.
              stopEffect(S_BEEPS);
              playEffect(S_BEEPS);

              stopEffect(S_VOICE_LEVEL_1);
              stopEffect(S_VOICE_LEVEL_2);
              stopEffect(S_VOICE_LEVEL_3);
              stopEffect(S_VOICE_LEVEL_4);
              stopEffect(S_VOICE_LEVEL_5);

              playEffect(S_VOICE_LEVEL_1);
            break;

            case MENU_LEVEL_1:
            default:
              // Cannot go any further than menu level 1.
              i_device_menu = 5;
            break;
          }
        }
        else {
          i_device_menu++;
        }
      }
    break;

    case ACTION_SETTINGS:
      // Counter clockwise.
      if(encoder.STATE == ENCODER_CCW) {
        if(i_device_menu == 4 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && switch_intensify.on() && !switch_grip.on()) {
          // No-op
        }
        else if(i_device_menu == 3 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && switch_intensify.on() && !switch_grip.on()) {
          // Lower the sound effects volume.
          decreaseVolumeEffects();
        }
        else if(i_device_menu == 3 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && !switch_intensify.on() && switch_grip.on() && b_playing_music) {
          // Decrease the music volume.
          decreaseVolumeMusic();
        }
        else if(i_device_menu - 1 < 1) {
          // We are entering the sub menu. Only accessible when the Single-Shot Blaster is powered down.
          if(DEVICE_STATUS == MODE_OFF) {
            switch(DEVICE_MENU_LEVEL) {
              case MENU_LEVEL_1:
                DEVICE_MENU_LEVEL = MENU_LEVEL_2;
                i_device_menu = 5;

                // Turn on the slo blow led to indicate we are in the Single-Shot Blaster sub menu.
                led_SloBlo.turnOn();

                // Play an indication beep to notify we have changed menu levels.
                stopEffect(S_BEEPS);
                playEffect(S_BEEPS);

                stopEffect(S_VOICE_LEVEL_1);
                stopEffect(S_VOICE_LEVEL_2);
                stopEffect(S_VOICE_LEVEL_3);
                stopEffect(S_VOICE_LEVEL_4);
                stopEffect(S_VOICE_LEVEL_5);

                playEffect(S_VOICE_LEVEL_2);
              break;

              case MENU_LEVEL_2:
              default:
                // Cannot go further than level 2 for this menu.
                i_device_menu = 1;
              break;
            }
          }
          else {
            i_device_menu = 1;
          }
        }
        else {
          i_device_menu--;
        }
      }

      // Clockwise.
      if(encoder.STATE == ENCODER_CW) {
        if(i_device_menu == 4 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && switch_intensify.on() && !switch_grip.on()) {
          // No-op
        }
        else if(i_device_menu == 3 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && switch_intensify.on() && !switch_grip.on()) {
          // Increase sound effects volume.
          increaseVolumeEffects();
        }
        else if(i_device_menu == 3 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && !switch_intensify.on() && switch_grip.on() && b_playing_music) {
          // Increase music volume.
          increaseVolumeMusic();
        }
        else if(i_device_menu + 1 > 5) {
          // We are leaving changing menu levels. Only accessible when the Single-Shot Blaster is powered down.
          if(DEVICE_STATUS == MODE_OFF) {
            switch(DEVICE_MENU_LEVEL) {
              case MENU_LEVEL_2:
                DEVICE_MENU_LEVEL = MENU_LEVEL_1;

                i_device_menu = 1;

                // Turn off the slo blow led to indicate we are no longer in the Single-Shot Blaster sub menu.
                led_SloBlo.turnOff();

                // Play an indication beep to notify we have changed menu levels.
                stopEffect(S_BEEPS);
                playEffect(S_BEEPS);

                stopEffect(S_VOICE_LEVEL_1);
                stopEffect(S_VOICE_LEVEL_2);
                stopEffect(S_VOICE_LEVEL_3);
                stopEffect(S_VOICE_LEVEL_4);
                stopEffect(S_VOICE_LEVEL_5);

                playEffect(S_VOICE_LEVEL_1);
              break;

              case MENU_LEVEL_1:
              default:
                // Level 1 is the first menu and nothing above it.
                i_device_menu = 5;
              break;
            }
          }
          else {
            i_device_menu = 5;
          }
        }
        else {
          i_device_menu++;
        }
      }
    break;

    default:
      if(DEVICE_STATUS == MODE_ON && switch_intensify.on() && !switch_vent.on() && !switch_device.on()) {
          // Counter clockwise.
          if(encoder.STATE == ENCODER_CCW) {
            // Decrease the master system volume.
            decreaseVolume();
          }
          else if(encoder.STATE == ENCODER_CW) {
            // Increase the master system volume.
            increaseVolume();
          }
      }
      else {
        if(DEVICE_ACTION_STATUS == ACTION_FIRING && POWER_LEVEL == LEVEL_5) {
          // Do nothing, we are locked in full power level while firing.
        }
        // Counter clockwise.
        else if(encoder.STATE == ENCODER_CCW) {
          if(switch_device.on() && switch_vent.on() && switch_activate.on()) {
            // Check to see the minimal power level depending on which system mode.
            if(decreasePowerLevel() && DEVICE_STATUS == MODE_ON) {
              soundIdleLoopStop();
              soundIdleLoop(false);
            }
          }
          else if(!switch_device.on() && switch_vent.on() && ms_firing_mode_switch.remaining() < 1 && DEVICE_STATUS == MODE_ON) {
            // Counter clockwise firing mode selection.
            STREAM_MODE = PROTON;
            ms_firing_mode_switch.start(i_firing_mode_switch_delay);
          }

          // Decrease the music volume if the device is off. A quick easy way to adjust the music volume on the go.
          if(DEVICE_STATUS == MODE_OFF && b_playing_music && !switch_intensify.on()) {
            decreaseVolumeMusic();
          }
          else if(DEVICE_STATUS == MODE_OFF && switch_intensify.on()) {
            // Decrease the master volume of the device.
            decreaseVolume();
          }
        }

        if(DEVICE_ACTION_STATUS == ACTION_FIRING && POWER_LEVEL == LEVEL_5) {
          // Do nothing, we are locked in full power level while firing.
        }
        // Clockwise.
        else if(encoder.STATE == ENCODER_CW) {
          if(switch_device.on() && switch_vent.on() && switch_activate.on()) {
            if(increasePowerLevel() && DEVICE_STATUS == MODE_ON) {
              soundIdleLoopStop();
              soundIdleLoop(false);
            }
          }
          else if(!switch_device.on() && switch_vent.on() && ms_firing_mode_switch.remaining() < 1 && DEVICE_STATUS == MODE_ON) {
            ms_firing_mode_switch.start(i_firing_mode_switch_delay);
          }

          // Increase the music volume if the device is off. A quick easy way to adjust the music volume on the go.
          if(DEVICE_STATUS == MODE_OFF && b_playing_music && !switch_intensify.on()) {
            increaseVolumeMusic();
          }
          else if(DEVICE_STATUS == MODE_OFF && switch_intensify.on()) {
            // Increase the master volume of the Single-Shot Blaster only.
            increaseVolume();
          }
        }
      }
    break;
  }
}

void vibrationDevice(uint8_t i_level) {
  if(b_vibration_enabled && i_level > 0) {
    // Vibrate the device during firing only when enabled.
    if(b_vibration_firing) {
      if(DEVICE_ACTION_STATUS == ACTION_FIRING || (ms_semi_automatic_firing.isRunning() && !ms_semi_automatic_firing.justFinished())) {
        if(ms_semi_automatic_firing.isRunning()) {
          analogWrite(vibration, 180);
        }
        else if(i_level != i_vibration_level_prev) {
          i_vibration_level_prev = i_level;
          analogWrite(vibration, i_level);
        }
      }
      else {
        vibrationOff();
      }
    }
    else {
      // Device vibrates even when idling, etc.
      if(i_level != i_vibration_level_prev) {
        i_vibration_level_prev = i_level;
        analogWrite(vibration, i_level);
      }
    }
  }
  else {
    vibrationOff();
  }
}

void vibrationSetting() {
  if(DEVICE_ACTION_STATUS != ACTION_FIRING) {
    switch(POWER_LEVEL) {
      case LEVEL_1:
      default:
        vibrationDevice(i_vibration_level);
      break;

      case LEVEL_2:
        vibrationDevice(i_vibration_level + 5);
      break;

      case LEVEL_3:
        vibrationDevice(i_vibration_level + 10);
      break;

      case LEVEL_4:
        vibrationDevice(i_vibration_level + 12);
      break;

      case LEVEL_5:
        vibrationDevice(i_vibration_level + 25);
      break;
    }
  }
}

void checkMenuVibration() {
  if(ms_menu_vibration.justFinished()) {
    vibrationOff();
  }
  else if(ms_menu_vibration.isRunning()) {
    analogWrite(vibration, 150);
  }
}

void vibrationOff() {
  ms_menu_vibration.stop();
  i_vibration_level_prev = 0;
  analogWrite(vibration, 0);
}

void switchLoops() {
  switch_intensify.poll();
  switch_activate.poll();
  switch_vent.poll();
  switch_device.poll();
  switch_grip.poll();
}

void ventSwitched(void* n) {
  (void)(n); // Suppress unused variable warning
  ventSwitchedCount++;
}

void deviceSwitched(void* n) {
  (void)(n); // Suppress unused variable warning
  deviceSwitchedCount++;
}

// Exit the device menu system while the device is off.
void deviceExitMenu() {
  i_device_menu = 5;

  playEffect(S_CLICK);

  DEVICE_ACTION_STATUS = ACTION_IDLE;

  allLightsOff();
}

// Exit the device menu EEPROM system while the device is off.
void deviceExitEEPROMMenu() {
  playEffect(S_BEEPS);

  deviceSwitchedCount = 0;
  ventSwitchedCount = 0;

  vibrationOff(); // Make sure we stop any menu-related vibration, if any.

  i_device_menu = 5;

  DEVICE_ACTION_STATUS = ACTION_IDLE;

  allLightsOff();
}

// Included last as the contained logic will control all aspects of the device using the defined functions above.
#include "Actions.h"
