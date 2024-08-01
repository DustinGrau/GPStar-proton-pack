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
  BARGRAPH_MODE = BARGRAPH_SUPER_HERO;
  BARGRAPH_FIRING_ANIMATION = BARGRAPH_ANIMATION_SUPER_HERO;
  VIBRATION_MODE_EEPROM = VIBRATION_ALWAYS;
  DEVICE_MENU_LEVEL = MENU_LEVEL_1;

  // Set callback events for these toggles, which need to count the activations for EEPROM menu entry.
  switch_vent.setPushedCallback(&ventSwitched);
  switch_device.setPushedCallback(&deviceSwitched);

  // Rotary encoder on the top of the device.
  pinModeFast(r_encoderA, INPUT_PULLUP);
  pinModeFast(r_encoderB, INPUT_PULLUP);

  delay(10); // Allow everything time to get set up.

  Wire.begin();
  Wire.setClock(400000UL); // Sets the i2c bus to 400kHz

  byte by_error, by_address;
  uint8_t i_i2c_devices = 0;

  // Scan i2c for any devices (28 segment bargraph).
  for(by_address = 1; by_address < 127; by_address++ ) {
    Wire.beginTransmission(by_address);
    by_error = Wire.endTransmission();

    if(by_error == 0) {
      i_i2c_devices++;
    }
  }

  if(i_i2c_devices > 0) {
    b_28segment_bargraph = true;
  }
  else {
    b_28segment_bargraph = false;
  }

  if(b_28segment_bargraph) {
    ht_bargraph.begin(0x00);
  }

  pinModeFast(led_slo_blo, OUTPUT); // Lower LED under the bargraph.
  pinModeFast(led_front_left, OUTPUT); // Front left LED underneath the Clippard valve.
  pinModeFast(led_hat_1, OUTPUT); // Hat light at front of barrel not used.
  pinModeFast(led_hat_2, OUTPUT); // Hat light at top of the device body (gun box).
  pinModeFast(led_barrel_tip, OUTPUT); // LED at the tip of the device barrel.
  pinMode(led_vent, OUTPUT); // Vent light could be either Digital or PWM based on user setting, so use default functions.
  pinModeFast(led_white, OUTPUT); // Alternative barrel tip light.

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
  // Play a sound to test the audio system.
  playEffect(S_DEVICE_READY);

  // These go HIGH to turn on.
  digitalWriteFast(led_slo_blo, HIGH);
  delay(100);
  digitalWriteFast(led_front_left, HIGH);
  delay(100);
  digitalWriteFast(led_hat_2, HIGH);
  delay(100);

  // These go LOW to turn on.
  digitalWrite(led_vent, LOW);
  delay(100);
  digitalWriteFast(led_white, LOW);
  delay(100);

  deviceTipOn();
  delay(100);

  // Sequentially turn on all LEDs in the cyclotron.
  for(uint8_t i = 0; i < i_num_cyclotron_leds; i++) {
    system_leds[i] = getHueAsRGB(C_RED);
    FastLED.show();
    delay(200);
  }

  // Turn on the front barrel.
  system_leds[i_barrel_led] = getHueAsRGB(C_WHITE);
  FastLED.show();

  delay(1000);

  allLightsOff();
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

        bargraphClearAlt();
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

            ms_bargraph.stop();
            bargraphClearAlt();

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
            digitalWriteFast(led_front_left, LOW);
          }
          else {
            digitalWriteFast(led_front_left, HIGH);
          }
        }
      }
    break;

    case MODE_ERROR:
      if(ms_hat_2.remaining() < i_hat_2_delay / 2) {
        digitalWriteFast(led_white, HIGH);

        digitalWriteFast(led_slo_blo, LOW);

        digitalWriteFast(led_hat_2, LOW);
        digitalWriteFast(led_front_left, LOW);
      }
      else {
        digitalWriteFast(led_hat_2, HIGH);
        digitalWriteFast(led_front_left, HIGH);

        digitalWriteFast(led_white, LOW);
        digitalWriteFast(led_slo_blo, HIGH);
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

      settingsBlinkingLights();
    break;

    case MODE_ON:
      if(!ms_hat_1.isRunning() && !ms_hat_2.isRunning()) {
        // Hat 2 stays solid while the Single-Shot Blaster is on.
        digitalWriteFast(led_hat_2, HIGH);
      }

      // Top white light.
      if(ms_white_light.justFinished()) {
        ms_white_light.repeat();
        if(digitalReadFast(led_white) == LOW) {
          digitalWriteFast(led_white, HIGH);
        }
        else {
          digitalWriteFast(led_white, LOW);
        }
      }

      // Ramp the bargraph up then ramp down back to the default power level setting on a fresh start.
      if(ms_bargraph.justFinished()) {
        bargraphRampUp();
      }
      else if(!ms_bargraph.isRunning() && DEVICE_ACTION_STATUS != ACTION_FIRING && DEVICE_ACTION_STATUS != ACTION_SETTINGS) {
        // Bargraph idling loop.
        bargraphPowerCheck();
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

  // Play the firing effect end animation.
  if(ms_firing_effect_end.justFinished()) {
    fireEffectEnd();
  }

  // Play the firing stream end animation.
  if(ms_firing_lights_end.justFinished()) {
    fireStreamEnd(getHueAsRGB(C_BLACK));
  }

  // Update the barrel LEDs and restart the timer.
  if(ms_fast_led.justFinished()) {
    FastLED.show();
    ms_fast_led.start(i_fast_led_delay);
  }
}

void updateCyclotron(uint8_t i_colour) {
  static bool sb_toggle = false; // Static toggle to remain scoped to this function between calls
  static uint8_t si_brightness_in = 0; // Static brightness variable for fade-in effect
  static uint8_t si_brightness_out = i_cyclotron_max_brightness; // Static brightness variable for fade-out effect

  if(ms_cyclotron.justFinished()) {
    // Change the timing (delay) based on the power level selected.
    uint16_t i_dynamic_delay = i_base_cyclotron_delay - ((i_power_level - 1) * (i_base_cyclotron_delay - i_min_cyclotron_delay) / 4);
    ms_cyclotron.start(i_dynamic_delay);

    // Increment brightness for fade-in effect
    if(si_brightness_in < i_cyclotron_max_brightness) {
      si_brightness_in += i_cyc_fade_step;
      if(si_brightness_in > i_cyclotron_max_brightness) {
        si_brightness_in = i_cyclotron_max_brightness;
      }
    }

    // Decrement brightness for fade-out effect
    if(si_brightness_out > 0) {
      si_brightness_out -= i_cyc_fade_step;
      if(si_brightness_out < 0) {
        si_brightness_out = 0;
      }
    }

    // Toggle between the LEDs in the i_cyclotron_pair using the given color.
    CRGB c_temp = getHueAsRGB(i_colour);
    if(sb_toggle) {
      system_leds[i_cyclotron_pair[0]] = c_temp.nscale8(si_brightness_in);  // Turn on LED 1 in the pair
      system_leds[i_cyclotron_pair[1]] = c_temp.nscale8(si_brightness_out); // Turn off LED 2 in the pair
    }
    else {
      system_leds[i_cyclotron_pair[0]] = c_temp.nscale8(si_brightness_out); // Turn off LED 1 in the pair
      system_leds[i_cyclotron_pair[1]] = c_temp.nscale8(si_brightness_in);  // Turn on LED 2 in the pair
    }

    // Toggle state and reset brightness variables after fade-in is complete.
    if (si_brightness_in == i_cyclotron_max_brightness && si_brightness_out == 0) {
      sb_toggle = !sb_toggle;
      si_brightness_in = 0;
      si_brightness_out = i_cyclotron_max_brightness;
    }
  }
}

void deviceTipOn() {
    // Illuminate the device barrel tip LED.
    digitalWriteFast(led_barrel_tip, HIGH);
}

void deviceTipOff() {
    // Turn off the device barrel tip LED.
    digitalWriteFast(led_barrel_tip, LOW);
}

void deviceTipSpark() {
  i_heatup_counter = 0;
  i_heatdown_counter = 100;
  i_bmash_spark_index = 0;
  ms_device_heatup_fade.start(i_delay_heatup);
}

void settingsBlinkingLights() {
  if(ms_settings_blinking.justFinished()) {
     ms_settings_blinking.start(i_settings_blinking_delay);
  }

  if(ms_settings_blinking.remaining() < i_settings_blinking_delay / 2) {
    bool b_solid_five = false;
    bool b_solid_one = false;

    // Indicator for looping track setting.
    if(b_repeat_track && i_device_menu == 5 && DEVICE_ACTION_STATUS != ACTION_ERROR && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && DEVICE_ACTION_STATUS != ACTION_CONFIG_EEPROM_MENU) {
      b_solid_five = true;
    }

    if(i_volume_master == i_volume_abs_min && DEVICE_ACTION_STATUS == ACTION_SETTINGS && DEVICE_MENU_LEVEL == MENU_LEVEL_1) {
      b_solid_one = true;
    }

    if(b_28segment_bargraph) {
      if(b_solid_five) {
        for(uint8_t i = 0; i < 24; i++) {
          if(b_solid_one && i < 3) {
            ht_bargraph.setLed(bargraphLookupTable(i));
            b_bargraph_status[i] = true;
          }
          else {
            ht_bargraph.clearLed(bargraphLookupTable(i));
            b_bargraph_status[i] = false;
          }
        }

        for(uint8_t i = 24; i < i_bargraph_segments - 1; i++) {
          ht_bargraph.setLed(bargraphLookupTable(i));
          b_bargraph_status[i] = true;
        }

        ht_bargraph.clearLed(bargraphLookupTable(27));
        b_bargraph_status[27] = false;

        ht_bargraph.sendLed(); // Commit the changes.
      }
      else if(b_solid_one) {
        for(uint8_t i = 0; i < i_bargraph_segments; i++) {
          if(i < 3) {
            ht_bargraph.setLed(bargraphLookupTable(i));
            b_bargraph_status[i] = true;
          }
          else {
            ht_bargraph.clearLed(bargraphLookupTable(i));
            b_bargraph_status[i] = false;
          }
        }

        ht_bargraph.sendLed(); // Commit the changes.
      }
      else {
        bargraphClearAll();
      }
    }
    else {
      if(b_solid_one) {
        digitalWriteFast(bargraphLookupTable(1-1), LOW);
      }
      else {
        digitalWriteFast(bargraphLookupTable(1-1), HIGH);
      }

      digitalWriteFast(bargraphLookupTable(2-1), HIGH);
      digitalWriteFast(bargraphLookupTable(3-1), HIGH);
      digitalWriteFast(bargraphLookupTable(4-1), HIGH);

      if(b_solid_five) {
        digitalWriteFast(bargraphLookupTable(5-1), LOW);
      }
      else {
        digitalWriteFast(bargraphLookupTable(5-1), HIGH);
      }
    }
  }
  else {
    switch(i_device_menu) {
      case 5:
        if(b_28segment_bargraph) {
          for(uint8_t i = 0; i < i_bargraph_segments; i++) {
            switch(i) {
              case 3:
              case 4:
              case 5:
              case 9:
              case 10:
              case 11:
              case 15:
              case 16:
              case 17:
              case 21:
              case 22:
              case 23:
              case 27:
                // Nothing
              break;

              default:
                ht_bargraph.setLed(bargraphLookupTable(i));
                b_bargraph_status[i] = true;
              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;

      case 4:
        if(b_28segment_bargraph) {
          for(uint8_t i = 0; i < 21; i++) {
            switch(i) {
              case 3:
              case 4:
              case 5:
              case 9:
              case 10:
              case 11:
              case 15:
              case 16:
              case 17:
              case 21:
              case 22:
              case 23:
              case 27:
                // Nothing
              break;

              default:
                ht_bargraph.setLed(bargraphLookupTable(i));
                b_bargraph_status[i] = true;
              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;

      case 3:
        if(b_28segment_bargraph) {
          for(uint8_t i = 0; i < 16; i++) {
            switch(i) {
              case 3:
              case 4:
              case 5:
              case 9:
              case 10:
              case 11:
              case 15:
              case 16:
              case 17:
              case 21:
              case 22:
              case 23:
              case 27:
                // Nothing
              break;

              default:
                ht_bargraph.setLed(bargraphLookupTable(i));
                b_bargraph_status[i] = true;
              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;

      case 2:
        if(b_28segment_bargraph) {
          for(uint8_t i = 0; i < 12; i++) {
            switch(i) {
              case 3:
              case 4:
              case 5:
              case 9:
              case 10:
              case 11:
              case 15:
              case 16:
              case 17:
              case 21:
              case 22:
              case 23:
              case 27:
                // Nothing
              break;

              default:
                ht_bargraph.setLed(bargraphLookupTable(i));
                b_bargraph_status[i] = true;
              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;

      case 1:
        if(b_28segment_bargraph) {
          for(uint8_t i = 0; i < 6; i++) {
            switch(i) {
              case 3:
              case 4:
              case 5:
              case 9:
              case 10:
              case 11:
              case 15:
              case 16:
              case 17:
              case 21:
              case 22:
              case 23:
              case 27:
                // Nothing
              break;

              default:
                ht_bargraph.setLed(bargraphLookupTable(i));
                b_bargraph_status[i] = true;
              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;
    }
  }
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
        analogWrite(led_vent, 0); // 0 = Full Power
      }
      else {
        // Adjust brightness based on the power level.
        switch(i_power_level) {
          case 5:
            analogWrite(led_vent, 100);
          break;
          case 4:
            analogWrite(led_vent, 130);
          break;
          case 3:
            analogWrite(led_vent, 160);
          break;
          case 2:
            analogWrite(led_vent, 190);
          break;
          case 1:
          default:
            analogWrite(led_vent, 220);
          break;
        }
      }
    }
    else {
      digitalWrite(led_vent, LOW);
    }
  }
  else if(!switch_vent.on()) {
    // Vent light and top white light off.
    digitalWrite(led_vent, HIGH);
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
      // Turn off additional timers.
      ms_bargraph.stop();
      ms_bargraph_alt.stop();

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
      switch(i_power_level) {
        case 5:
          ms_bmash.start(i_bmash_cool_down + 2000);
        break;

        case 4:
          ms_bmash.start(i_bmash_cool_down + 1500);
        break;

        case 3:
          ms_bmash.start(i_bmash_cool_down + 1000);
        break;

        case 2:
          ms_bmash.start(i_bmash_cool_down + 500);
        break;

        case 1:
        default:
          ms_bmash.start(i_bmash_cool_down);
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
          bargraphClearAlt();
        }
        else {
          DEVICE_ACTION_STATUS = ACTION_IDLE;
          ms_settings_blinking.stop();
          bargraphClearAlt();
        }

        playEffect(S_CLICK);
      }
    }
    else if(DEVICE_ACTION_STATUS == ACTION_SETTINGS && switch_vent.on() && switch_device.on()) {
      // Exit the settings menu if the user turns the device switch back on.
      DEVICE_ACTION_STATUS = ACTION_IDLE;
      ms_settings_blinking.stop();
      bargraphClearAlt();
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
  i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_2021;

  if(DEVICE_STATUS != MODE_ERROR) {
    bargraphRampUp();
    if(switch_vent.on()) {
      b_all_switch_activation = true; // If vent switch is already on when Activate is flipped, set to true for soundIdleLoop() to use
    }

    // Turn on slo-blo light.
    digitalWriteFast(led_slo_blo, HIGH);

    // Turn on the Clippard LED.
    digitalWriteFast(led_front_left, HIGH);

    // Top white light.
    ms_white_light.start(i_top_blink_interval);
    digitalWriteFast(led_white, LOW);

    // Reset the hat light timers.
    ms_hat_1.stop();
    ms_hat_2.stop();

    stopEffect(S_BOOTUP);
    playEffect(S_BOOTUP);

    soundIdleLoop(true);
  }
}

void soundIdleLoop(bool fadeIn) {
  switch(i_power_level) {
    case 1:
    default:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;

    case 2:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;

    case 3:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;

    case 4:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;

    case 5:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 3000);
    break;
  }
}

void soundIdleLoopStop() {
  switch(i_power_level) {
    case 1:
    default:
      stopEffect(S_IDLE_LOOP);
    break;

    case 2:
      stopEffect(S_IDLE_LOOP);
    break;

    case 3:
      stopEffect(S_IDLE_LOOP);
    break;

    case 4:
      stopEffect(S_IDLE_LOOP);
    break;

    case 5:
      stopEffect(S_IDLE_LOOP);
    break;
  }
}

void modePulseStart() {
  // Handles all "pulsed" fire modes.
  i_fast_led_delay = FAST_LED_UPDATE_MS;
  barrelLightsOff();

  switch(STREAM_MODE) {
    case PROTON:
      // Primary Blast.
      playEffect(S_FIRE_BLAST, false, i_volume_effects, false, 0, false);
      ms_firing_pulse.start(0);
      ms_semi_automatic_firing.start(350);
    break;

    default:
      // Do nothing.
    break;
  }
}

void modeFireStart() {
  i_fast_led_delay = FAST_LED_UPDATE_MS;

  //modeFireStartSounds();

  // Just in case a semi-auto was fired before we started firing a stream, stop its timer.
  ms_semi_automatic_firing.stop();

  switch(BARGRAPH_FIRING_ANIMATION) {
    case BARGRAPH_ANIMATION_ORIGINAL:
      // Redraw the bargraph to the current power level before doing the MODE_ORIGINAL firing animation.
      bargraphRedraw();

      // Reset the Hasbro bargraph.
      i_bargraph_status = 0;
    break;

    case BARGRAPH_ANIMATION_SUPER_HERO:
    default:
      // Clear the bargraph before we do the animation.
      bargraphClearAlt();

      // Reset the Hasbro bargraph.
      i_bargraph_status = 1;
    break;
  }

  // Turn on hat light 1.
  digitalWriteFast(led_hat_1, HIGH);

  barrelLightsOff();

  ms_firing_lights.start(0);

  // Stop any bargraph ramps.
  ms_bargraph.stop();

  ms_bargraph_alt.stop();

  // Reset the 28 segment bargraph.
  i_bargraph_status_alt = 0;

  b_bargraph_up = false;

  bargraphRampFiring();

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

  ms_bargraph_firing.stop();

  ms_bargraph_alt.stop(); // Stop the 1984 28 segment optional bargraph timer.
  b_bargraph_up = false;

  switch(BARGRAPH_MODE) {
    case BARGRAPH_SUPER_HERO:
    default:
      i_bargraph_status = i_power_level - 1;
      i_bargraph_status_alt = 0;
      bargraphClearAlt();

      i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_2021;

      // We ramp the bargraph back up after finishing firing.
      bargraphRampUp();
    break;
  }

  ms_firing_lights.stop();
  ms_impact.stop();
  ms_firing_sound_mix.stop();
  ms_firing_effect_end.start(0);

  digitalWriteFast(led_hat_2, HIGH); // Make sure we turn on hat light 2 in case it's off as well.

  deviceTipOff();

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

  // Initialize temporary colour variables to reduce code complexity.
  colours c_temp_start = C_WHITE;
  colours c_temp_effect = C_WHITE;

  switch(STREAM_MODE) {
    case PROTON:
    default:
      // Shift the stream from red to orange on higher power levels.
      switch(i_power_level) {
        case 1:
        default:
          c_temp_start = C_RED;
          c_temp_effect = C_BLUE;
        break;

        case 2:
          c_temp_start = C_RED2;
          c_temp_effect = C_BLUE;
        break;

        case 3:
          c_temp_start = C_RED3;
          c_temp_effect = C_LIGHT_BLUE;
        break;

        case 4:
          c_temp_start = C_RED4;
          c_temp_effect = C_LIGHT_BLUE;
        break;

        case 5:
          c_temp_start = C_RED5;
          c_temp_effect = C_WHITE;
        break;
      }
    break;
  }

  fireStreamStart(getHueAsRGB(c_temp_start));
  fireStreamEffect(getHueAsRGB(c_temp_effect));

  // Bargraph loop / scroll.
  if(ms_bargraph_firing.justFinished()) {
    bargraphRampFiring();
  }
}

void firePulseEffect() {
  if(i_pulse_step < 1) {
    // Play bargraph animation when pulse sequence begins.
    switch(BARGRAPH_MODE) {
      case BARGRAPH_SUPER_HERO:
      default:
        i_bargraph_status = i_power_level - 1;
        i_bargraph_status_alt = 0;
        bargraphClearAlt();

        i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_2021;

        // We ramp the bargraph back up after finishing firing.
        bargraphRampUp();
      break;
    }
  }

  uint8_t i_firing_pulse = d_firing_pulse; // Stores a calculated value based on firing mode.

  switch(i_pulse_step) {
    case 0:
      switch(STREAM_MODE) {
        case PROTON:
        default:
          // Primary Blast.
          system_leds[i_barrel_led] = getHueAsRGB(C_RED);
        break;
      }
    break;

    case 1:
      switch(STREAM_MODE) {
        case PROTON:
        default:
          // Primary Blast.
          system_leds[i_barrel_led] = getHueAsRGB(C_RED2);
        break;
      }
    break;

    case 2:
      switch(STREAM_MODE) {
        case PROTON:
        default:
          // Primary Blast.
          system_leds[i_barrel_led] = getHueAsRGB(C_WHITE);
        break;
      }
    break;

    case 3:
      switch(STREAM_MODE) {
        case PROTON:
        default:
          // Primary Blast.
          system_leds[i_barrel_led] = getHueAsRGB(C_RED2);
        break;
      }
    break;

    case 4:
      switch(STREAM_MODE) {
        case PROTON:
        default:
          // Primary Blast.
          system_leds[i_barrel_led] = getHueAsRGB(C_RED);

          // Bolt has reached barrel tip, so turn on tip light.
          deviceTipOn();
        break;
      }
    break;

    case 5:
      switch(STREAM_MODE) {
        case PROTON:
        default:
          // Primary Blast.
          system_leds[i_barrel_led] = getHueAsRGB(C_BLACK);
        break;
      }
    break;

    default:
      // Do nothing if we somehow end up here.
    break;
  }

  i_pulse_step++;

  if(i_pulse_step < i_pulse_step_max) {
    if(STREAM_MODE == PROTON) {
      // Primary Blast is much slower than the others.
      i_firing_pulse *= 2;
    }
    ms_firing_pulse.start(i_firing_pulse);
  }
  else {
    // Animation has concluded, so reset our timer and variable.
    deviceTipOff();
    ms_firing_pulse.stop();
    i_pulse_step = 0;
  }
}

void fireStreamEffect(CRGB c_colour) {
  uint8_t i_firing_stream; // Stores a calculated value based on LED count.

  i_firing_stream = d_firing_stream;

  if(ms_firing_stream_effects.justFinished()) {
    if(i_cyclotron_light - 1 >= 0 && i_cyclotron_light - 1 < i_num_cyclotron_leds) {
      switch(STREAM_MODE) {
        case PROTON:
        default:
          // Shift the stream from red to orange on higher power levels.
          switch(i_power_level) {
            case 1:
            default:
              system_leds[i_cyclotron_light - 1] = getHueAsRGB(C_RED);
            break;

            case 2:
              system_leds[i_cyclotron_light - 1] = getHueAsRGB(C_RED2);
            break;

            case 3:
              system_leds[i_cyclotron_light - 1] = getHueAsRGB(C_RED3);
            break;

            case 4:
              system_leds[i_cyclotron_light - 1] = getHueAsRGB(C_RED4);
            break;

            case 5:
              system_leds[i_cyclotron_light - 1] = getHueAsRGB(C_RED5);
            break;
          }
        break;
      }
    }

    if(i_cyclotron_light == i_num_cyclotron_leds) {
      i_cyclotron_light = 0;

      switch(STREAM_MODE) {
        default:
          switch(i_power_level) {
            case 1:
            default:
              ms_firing_stream_effects.start(i_firing_stream); // 100ms
            break;

            case 2:
              ms_firing_stream_effects.start(i_firing_stream - 15); // 85ms
            break;

            case 3:
              ms_firing_stream_effects.start(i_firing_stream - 30); // 70ms
            break;

            case 4:
              ms_firing_stream_effects.start(i_firing_stream - 45); // 55ms
            break;

            case 5:
              ms_firing_stream_effects.start(i_firing_stream - 60); // 40ms
            break;
          }
        break;
      }
    }
    else if(i_cyclotron_light < i_num_cyclotron_leds) {
      system_leds[i_cyclotron_light] = c_colour;

      switch(STREAM_MODE) {
        default:
          switch(i_power_level) {
            case 1:
            default:
              ms_firing_stream_effects.start((d_firing_stream / 5) + 10); // 30ms
            break;

            case 2:
              ms_firing_stream_effects.start((d_firing_stream / 5) + 8); // 28ms
            break;

            case 3:
              ms_firing_stream_effects.start((d_firing_stream / 5) + 6); // 26ms
            break;

            case 4:
              ms_firing_stream_effects.start((d_firing_stream / 5) + 5); // 25ms
            break;

            case 5:
              ms_firing_stream_effects.start((d_firing_stream / 5) + 4); // 24ms
            break;
          }
        break;
      }

      i_cyclotron_light++;
    }
  }
}

void barrelLightsOff() {
  ms_firing_pulse.stop();
  ms_firing_lights.stop();
  ms_firing_stream_effects.stop();
  ms_firing_effect_end.stop();
  ms_firing_lights_end.stop();
  ms_device_heatup_fade.stop();

  i_cyclotron_light = 0;
  i_pulse_step = 0;
  i_heatup_counter = 0;
  i_heatdown_counter = 100;

  // Turn off the barrel LED.
  system_leds[i_barrel_led] = getHueAsRGB(C_BLACK);

  // Turn off the device barrel tip LED.
  deviceTipOff();
}

void fireStreamStart(CRGB c_colour) {
  if(ms_firing_lights.justFinished() && i_cyclotron_light < i_num_cyclotron_leds) {
    // Just set the current LED to the expected colour.
    system_leds[i_cyclotron_light] = c_colour;

    // Firing at "normal" speed.
    ms_firing_lights.start(d_firing_stream / 5);

    i_cyclotron_light++;

    if(i_cyclotron_light == i_num_cyclotron_leds) {
      i_cyclotron_light = 0;

      ms_firing_lights.stop();

      // Firing at "normal" speed.
      ms_firing_stream_effects.start(d_firing_stream);
    }
  }
}

void fireEffectEnd() {
  // Initialize temporary colour variable to reduce code complexity.
  colours c_temp = C_WHITE;

  if(i_cyclotron_light < i_num_cyclotron_leds && ms_firing_stream_effects.isRunning()) {
    switch(STREAM_MODE) {
      case PROTON:
      default:
      // Shift the stream from red to orange on higher power levels.
      switch(i_power_level) {
        case 1:
        default:
          c_temp = C_BLUE;
        break;

        case 2:
          c_temp = C_BLUE;
        break;

        case 3:
          c_temp = C_LIGHT_BLUE;
        break;

        case 4:
          c_temp = C_LIGHT_BLUE;
        break;

        case 5:
          c_temp = C_WHITE;
        break;
      }
    }

    fireStreamEffect(getHueAsRGB(c_temp));

    if(i_cyclotron_light < i_num_cyclotron_leds) {
      ms_firing_effect_end.repeat();
    }
    else {
      // Give a slight delay for the final pixel before clearing it.
      uint8_t i_firing_stream; // Stores a calculated value based on LED count.

      // Firing at "normal" speed.
      i_firing_stream = d_firing_stream;

      switch(i_power_level) {
        case 1:
        default:
          // Do nothing.
        break;

        case 2:
          i_firing_stream = i_firing_stream - 15;
        break;

        case 3:
          i_firing_stream = i_firing_stream - 30;
        break;

        case 4:
          i_firing_stream = i_firing_stream - 45;
        break;

        case 5:
          i_firing_stream = i_firing_stream - 60;
        break;
      }

      ms_firing_effect_end.start(i_firing_stream);
      ms_firing_stream_effects.stop();
    }
  }
  else {
    switch(STREAM_MODE) {
      case PROTON:
      default:
        // Shift the stream from red to orange on higher power levels.
        switch(i_power_level) {
          case 1:
          default:
            c_temp = C_RED;
          break;

          case 2:
            c_temp = C_RED2;
          break;

          case 3:
            c_temp = C_RED3;
          break;

          case 4:
            c_temp = C_RED4;
          break;

          case 5:
            c_temp = C_RED5;
          break;
        }
      break;
    }

    // Set the final LED back to whatever colour it is without the effect.
    system_leds[i_cyclotron_light - 1] = getHueAsRGB(c_temp);

    i_cyclotron_light = 0;
    ms_firing_effect_end.stop();
    ms_firing_lights_end.start(0);
  }
}

void fireStreamEnd(CRGB c_colour) {
  if(i_cyclotron_light < i_num_cyclotron_leds) {
    // Set the colour for the specific LED.
    system_leds[i_cyclotron_light] = c_colour;

    // Firing at a "normal" rate
    ms_firing_lights_end.start(d_firing_stream / 5);

    i_cyclotron_light++;

    if(i_cyclotron_light == i_num_cyclotron_leds) {
      i_cyclotron_light = 0;

      // Turn off device tip in case it's still on.
      deviceTipOff();

      ms_firing_lights_end.stop();

      i_fast_led_delay = FAST_LED_UPDATE_MS;
    }
  }
}

// This is the Super Hero bargraph firing animation. Ramping up and down from the middle to the top/bottom and back to the middle again.
void bargraphSuperHeroRampFiringAnimation() {
  if(b_28segment_bargraph) {
    switch(i_bargraph_status_alt) {
      case 0:
        vibrationDevice(i_vibration_level + 110);

        ht_bargraph.setLed(bargraphLookupTable(13));
        ht_bargraph.setLed(bargraphLookupTable(14));

        b_bargraph_status[13] = true;
        b_bargraph_status[14] = true;

        i_bargraph_status_alt++;

        if(!b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(12));
          ht_bargraph.clearLed(bargraphLookupTable(15));

          b_bargraph_status[12] = false;
          b_bargraph_status[15] = false;
        }

        b_bargraph_up = true;

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOn();
      break;

      case 1:
        vibrationDevice(i_vibration_level + 110);

        ht_bargraph.setLed(bargraphLookupTable(12));
        ht_bargraph.setLed(bargraphLookupTable(15));

        b_bargraph_status[12] = true;
        b_bargraph_status[15] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(13));
          ht_bargraph.clearLed(bargraphLookupTable(14));

          b_bargraph_status[13] = false;
          b_bargraph_status[14] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(11));
          ht_bargraph.clearLed(bargraphLookupTable(16));

          b_bargraph_status[11] = false;
          b_bargraph_status[16] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOn();
      break;

      case 2:
        vibrationDevice(i_vibration_level + 110);

        ht_bargraph.setLed(bargraphLookupTable(11));
        ht_bargraph.setLed(bargraphLookupTable(16));

        b_bargraph_status[11] = true;
        b_bargraph_status[16] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(12));
          ht_bargraph.clearLed(bargraphLookupTable(15));

          b_bargraph_status[12] = false;
          b_bargraph_status[15] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(10));
          ht_bargraph.clearLed(bargraphLookupTable(17));

          b_bargraph_status[10] = false;
          b_bargraph_status[17] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOff();
      break;

      case 3:
        vibrationDevice(i_vibration_level + 110);

        ht_bargraph.setLed(bargraphLookupTable(10));
        ht_bargraph.setLed(bargraphLookupTable(17));

        b_bargraph_status[10] = true;
        b_bargraph_status[17] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(11));
          ht_bargraph.clearLed(bargraphLookupTable(16));

          b_bargraph_status[11] = false;
          b_bargraph_status[16] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(9));
          ht_bargraph.clearLed(bargraphLookupTable(18));

          b_bargraph_status[9] = false;
          b_bargraph_status[18] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOff();
      break;

      case 4:
        vibrationDevice(i_vibration_level + 110);

        ht_bargraph.setLed(bargraphLookupTable(9));
        ht_bargraph.setLed(bargraphLookupTable(18));

        b_bargraph_status[9] = true;
        b_bargraph_status[18] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(10));
          ht_bargraph.clearLed(bargraphLookupTable(17));

          b_bargraph_status[10] = false;
          b_bargraph_status[17] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(8));
          ht_bargraph.clearLed(bargraphLookupTable(19));

          b_bargraph_status[8] = false;
          b_bargraph_status[19] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOn();
      break;

      case 5:
        vibrationDevice(i_vibration_level + 110);

        ht_bargraph.setLed(bargraphLookupTable(8));
        ht_bargraph.setLed(bargraphLookupTable(19));

        b_bargraph_status[8] = true;
        b_bargraph_status[19] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(9));
          ht_bargraph.clearLed(bargraphLookupTable(18));

          b_bargraph_status[9] = false;
          b_bargraph_status[18] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(7));
          ht_bargraph.clearLed(bargraphLookupTable(20));

          b_bargraph_status[7] = false;
          b_bargraph_status[20] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOn();
      break;

      case 6:
        vibrationDevice(i_vibration_level + 112);

        ht_bargraph.setLed(bargraphLookupTable(7));
        ht_bargraph.setLed(bargraphLookupTable(20));

        b_bargraph_status[7] = true;
        b_bargraph_status[20] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(8));
          ht_bargraph.clearLed(bargraphLookupTable(19));

          b_bargraph_status[8] = false;
          b_bargraph_status[19] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(6));
          ht_bargraph.clearLed(bargraphLookupTable(21));

          b_bargraph_status[6] = false;
          b_bargraph_status[21] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOff();
      break;

      case 7:
        vibrationDevice(i_vibration_level + 112);

        ht_bargraph.setLed(bargraphLookupTable(6));
        ht_bargraph.setLed(bargraphLookupTable(21));

        b_bargraph_status[6] = true;
        b_bargraph_status[21] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(7));
          ht_bargraph.clearLed(bargraphLookupTable(20));

          b_bargraph_status[7] = false;
          b_bargraph_status[20] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(5));
          ht_bargraph.clearLed(bargraphLookupTable(22));

          b_bargraph_status[5] = false;
          b_bargraph_status[22] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOff();
      break;

      case 8:
        vibrationDevice(i_vibration_level + 112);

        ht_bargraph.setLed(bargraphLookupTable(5));
        ht_bargraph.setLed(bargraphLookupTable(22));

        b_bargraph_status[5] = true;
        b_bargraph_status[22] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(6));
          ht_bargraph.clearLed(bargraphLookupTable(21));

          b_bargraph_status[6] = false;
          b_bargraph_status[21] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(4));
          ht_bargraph.clearLed(bargraphLookupTable(23));

          b_bargraph_status[4] = false;
          b_bargraph_status[23] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOn();
      break;

      case 9:
        vibrationDevice(i_vibration_level + 112);

        ht_bargraph.setLed(bargraphLookupTable(4));
        ht_bargraph.setLed(bargraphLookupTable(23));

        b_bargraph_status[4] = true;
        b_bargraph_status[23] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(5));
          ht_bargraph.clearLed(bargraphLookupTable(22));

          b_bargraph_status[5] = false;
          b_bargraph_status[22] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(3));
          ht_bargraph.clearLed(bargraphLookupTable(24));

          b_bargraph_status[3] = false;
          b_bargraph_status[24] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOn();
      break;

      case 10:
        vibrationDevice(i_vibration_level + 112);

        ht_bargraph.setLed(bargraphLookupTable(3));
        ht_bargraph.setLed(bargraphLookupTable(24));

        b_bargraph_status[3] = true;
        b_bargraph_status[24] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(4));
          ht_bargraph.clearLed(bargraphLookupTable(23));

          b_bargraph_status[4] = false;
          b_bargraph_status[23] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(2));
          ht_bargraph.clearLed(bargraphLookupTable(25));

          b_bargraph_status[2] = false;
          b_bargraph_status[25] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOff();
      break;

      case 11:
        vibrationDevice(i_vibration_level + 115);

        ht_bargraph.setLed(bargraphLookupTable(2));
        ht_bargraph.setLed(bargraphLookupTable(25));

        b_bargraph_status[2] = false;
        b_bargraph_status[25] = false;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(3));
          ht_bargraph.clearLed(bargraphLookupTable(24));

          b_bargraph_status[3] = false;
          b_bargraph_status[24] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(1));
          ht_bargraph.clearLed(bargraphLookupTable(26));

          b_bargraph_status[1] = false;
          b_bargraph_status[26] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOff();
      break;

      case 12:
        vibrationDevice(i_vibration_level + 115);

        ht_bargraph.setLed(bargraphLookupTable(1));
        ht_bargraph.setLed(bargraphLookupTable(26));

        b_bargraph_status[1] = true;
        b_bargraph_status[26] = true;

        if(b_bargraph_up) {
          ht_bargraph.clearLed(bargraphLookupTable(2));
          ht_bargraph.clearLed(bargraphLookupTable(25));

          b_bargraph_status[2] = false;
          b_bargraph_status[25] = false;

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLed(bargraphLookupTable(0));
          ht_bargraph.clearLed(bargraphLookupTable(27));

          b_bargraph_status[0] = false;
          b_bargraph_status[27] = false;

          i_bargraph_status_alt--;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOn();
      break;

      case 13:
        vibrationDevice(i_vibration_level + 115);

        ht_bargraph.setLed(bargraphLookupTable(0));
        ht_bargraph.setLed(bargraphLookupTable(27));

        b_bargraph_status[0] = true;
        b_bargraph_status[27] = true;

        ht_bargraph.clearLed(bargraphLookupTable(1));
        ht_bargraph.clearLed(bargraphLookupTable(26));

        b_bargraph_status[1] = false;
        b_bargraph_status[26] = false;

        i_bargraph_status_alt--;

        b_bargraph_up = false;

        ht_bargraph.sendLed(); // Commit the changes.
        deviceTipOn();
      break;
    }
  }
}

// This is the Mode Original bargraph firing animation. The top portion fluctuates during firing and becomes more erratic the longer firing continues.
void bargraphModeOriginalRampFiringAnimation() {
  if(b_28segment_bargraph) {
    /*
      5: full: 23 - 27  (5 segments)
      4: 3/4: 17 - 22   (6 segments)
      3: 1/2: 12 - 16   (5 segments)
      2: 1/4: 5 - 11    (7 segments)
      1: none: 0 - 4    (5 segments)
    */

    // When firing starts, i_bargraph_status_alt resets to 0 in modeFireStart();
    if(i_bargraph_status_alt == 0) {
      // Set our target.
      switch(i_power_level) {
        case 5:
          i_bargraph_status_alt = random(18, i_bargraph_segments - 1);
        break;

        case 4:
          i_bargraph_status_alt = random(13, 25);
        break;

        case 3:
          i_bargraph_status_alt = random(9, 19);
        break;

        case 2:
          i_bargraph_status_alt = random(3, 13);
        break;

        case 1:
        default:
          // Not used in MODE_ORIGINAL.
          //i_bargraph_status_alt = random(0, 6);
        break;
      }
    }

    bool b_tmp_down = true;

    for(uint8_t i = 0; i < i_bargraph_segments; i++) {
      if(!b_bargraph_status[i] && i < i_bargraph_status_alt) {
        b_tmp_down = false;
        break;
      }
    }

    switch(i_power_level) {
      case 5:
        if(b_tmp_down) {
          // Moving down.
          for(uint8_t i = i_bargraph_segments - 1; i >= i_bargraph_status_alt; i--) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(6, i_bargraph_segments - 1);
                break;

                case 4:
                  i_bargraph_status_alt = random(9, i_bargraph_segments - 1);
                break;

                case 3:
                  i_bargraph_status_alt = random(12, i_bargraph_segments - 1);
                break;

                case 2:
                  i_bargraph_status_alt = random(15, i_bargraph_segments - 1);
                break;

                case 1:
                default:
                  i_bargraph_status_alt = random(18, i_bargraph_segments - 1);
                break;
              }
            }

            if(b_bargraph_status[i]) {
              ht_bargraph.clearLed(bargraphLookupTable(i));
              b_bargraph_status[i] = false;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
        else {
          // Need to move up.
          for(uint8_t i = 0; i <= i_bargraph_status_alt; i++) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(8, i_bargraph_segments - 1);
                break;

                case 4:
                  i_bargraph_status_alt = random(12, i_bargraph_segments - 1);
                break;

                case 3:
                  i_bargraph_status_alt = random(14, i_bargraph_segments - 1);
                break;

                case 2:
                  i_bargraph_status_alt = random(16, i_bargraph_segments - 1);
                break;

                case 1:
                  i_bargraph_status_alt = random(18, i_bargraph_segments - 1);
                break;

                default:
                  i_bargraph_status_alt = random(0, i_bargraph_segments - 1);
                break;
              }
            }

            if(!b_bargraph_status[i]) {
              ht_bargraph.setLed(bargraphLookupTable(i));
              b_bargraph_status[i] = true;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;

      case 4:
        if(b_tmp_down) {
          // Moving down.
          for(uint8_t i = 25; i >= i_bargraph_status_alt; i--) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(1, 25);
                break;

                case 4:
                  i_bargraph_status_alt = random(4, 25);
                break;

                case 3:
                  i_bargraph_status_alt = random(7, 25);
                break;

                case 2:
                  i_bargraph_status_alt = random(10, 25);
                break;

                case 1:
                  i_bargraph_status_alt = random(13, 25);
                break;

                default:
                  i_bargraph_status_alt = random(0, 25);
                break;
              }
            }

            if(b_bargraph_status[i]) {
              ht_bargraph.clearLed(bargraphLookupTable(i));
              b_bargraph_status[i] = false;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
        else {
          // Need to move up.
          for(uint8_t i = 0; i <= i_bargraph_status_alt; i++) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(1, 25);
                break;

                case 4:
                  i_bargraph_status_alt = random(4, 25);
                break;

                case 3:
                  i_bargraph_status_alt = random(7, 25);
                break;

                case 2:
                  i_bargraph_status_alt = random(10, 25);
                break;

                case 1:
                  i_bargraph_status_alt = random(13, 25);
                break;

                default:
                  i_bargraph_status_alt = random(0, 25);
                break;
              }
            }

            if(!b_bargraph_status[i]) {
              ht_bargraph.setLed(bargraphLookupTable(i));
              b_bargraph_status[i] = true;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;

      case 3:
        if(b_tmp_down) {
          // Moving down.
          for(uint8_t i = 19; i >= i_bargraph_status_alt; i--) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(1, 19);
                break;

                case 4:
                  i_bargraph_status_alt = random(3, 19);
                break;

                case 3:
                  i_bargraph_status_alt = random(5, 19);
                break;

                case 2:
                  i_bargraph_status_alt = random(7, 19);
                break;

                case 1:
                  i_bargraph_status_alt = random(9, 19);
                break;

                default:
                  i_bargraph_status_alt = random(0, 19);
                break;
              }
            }

            if(b_bargraph_status[i]) {
              ht_bargraph.clearLed(bargraphLookupTable(i));
              b_bargraph_status[i] = false;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
        else {
          // Need to move up.
          for(uint8_t i = 0; i <= i_bargraph_status_alt; i++) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(1, 19);
                break;

                case 4:
                  i_bargraph_status_alt = random(3, 19);
                break;

                case 3:
                  i_bargraph_status_alt = random(5, 19);
                break;

                case 2:
                  i_bargraph_status_alt = random(7, 19);
                break;

                case 1:
                  i_bargraph_status_alt = random(9, 19);
                break;

                default:
                  i_bargraph_status_alt = random(0, 19);
                break;
              }
            }

            if(!b_bargraph_status[i]) {
              ht_bargraph.setLed(bargraphLookupTable(i));
              b_bargraph_status[i] = true;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;

      case 2:
        if(b_tmp_down) {
          // Moving down.
          for(uint8_t i = 13; i >= i_bargraph_status_alt; i--) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(1, 13);
                break;

                case 4:
                  i_bargraph_status_alt = random(2, 13);
                break;

                case 3:
                case 2:
                case 1:
                  i_bargraph_status_alt = random(3, 13);
                break;

                default:
                  i_bargraph_status_alt = random(0, 13);
                break;
              }
            }

            if(b_bargraph_status[i]) {
              ht_bargraph.clearLed(bargraphLookupTable(i));
              b_bargraph_status[i] = false;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
        else {
          // Need to move up.
          for(uint8_t i = 0; i <= i_bargraph_status_alt; i++) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(1, 13);
                break;

                case 4:
                  i_bargraph_status_alt = random(2, 13);
                break;

                case 3:
                case 2:
                case 1:
                  i_bargraph_status_alt = random(3, 13);
                break;

                default:
                  i_bargraph_status_alt = random(0, 13);
                break;
              }
            }

            if(!b_bargraph_status[i]) {
              ht_bargraph.setLed(bargraphLookupTable(i));
              b_bargraph_status[i] = true;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;

      case 1:
      default:
        if(b_tmp_down) {
          // Moving down.
          for(uint8_t i = 7; i >= i_bargraph_status_alt; i--) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(0, 10);
                break;

                case 4:
                  i_bargraph_status_alt = random(0, 9);
                break;

                case 3:
                case 2:
                case 1:
                  i_bargraph_status_alt = random(0, 8);
                break;

                default:
                  i_bargraph_status_alt = random(0, 7);
                break;
              }
            }

            if(b_bargraph_status[i]) {
              ht_bargraph.clearLed(bargraphLookupTable(i));
              b_bargraph_status[i] = false;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
        else {
          // Need to move up.
          for(uint8_t i = 0; i <= i_bargraph_status_alt; i++) {
            if(i_bargraph_status_alt == i) {
              switch(i_cyclotron_speed_up) {
                case 5:
                  i_bargraph_status_alt = random(0, 10);
                break;

                case 4:
                  i_bargraph_status_alt = random(0, 9);
                break;

                case 3:
                case 2:
                case 1:
                  i_bargraph_status_alt = random(0, 8);
                break;

                default:
                  i_bargraph_status_alt = random(0, 7);
                break;
              }
            }

            if(!b_bargraph_status[i]) {
              ht_bargraph.setLed(bargraphLookupTable(i));
              b_bargraph_status[i] = true;

              break;
            }
          }

          ht_bargraph.sendLed(); // Commit the changes.
        }
      break;
    }

    if(i_bargraph_status_alt > 22) {
      vibrationDevice(i_vibration_level + 115);
    }
    else if(i_bargraph_status_alt > 11) {
      vibrationDevice(i_vibration_level + 112);
    }
    else {
      vibrationDevice(i_vibration_level + 110);
    }
  }

  if(i_bargraph_status > 3) {
    vibrationDevice(i_vibration_level + 115);
  }
  else if(i_bargraph_status > 1) {
    vibrationDevice(i_vibration_level + 112);
  }
  else {
    vibrationDevice(i_vibration_level + 110);
  }
}

// Bargraph ramping during firing.
// Optional barrel LED tip strobing is controlled from here to give it a ramp effect if the device is going to overheat.
void bargraphRampFiring() {
  switch(BARGRAPH_FIRING_ANIMATION) {
    case BARGRAPH_ANIMATION_SUPER_HERO:
      bargraphSuperHeroRampFiringAnimation();
    break;
    case BARGRAPH_ANIMATION_ORIGINAL:
    default:
      bargraphModeOriginalRampFiringAnimation();

      // Strobe the optional tip light on even barrel LED numbers.
      if((i_cyclotron_light & 0x01) == 0) {
        deviceTipOn();
      }
      else {
        deviceTipOff();
      }
    break;
  }

  uint8_t i_ramp_interval = d_bargraph_ramp_interval;

  if(b_28segment_bargraph) {
    // Switch to a different ramp speed if using the (Optional) 28 segment barmeter bargraph.
    i_ramp_interval = d_bargraph_ramp_interval_alt;
  }

  // If in a power level on the device that can overheat, change the speed of the bargraph ramp during firing based on time remaining before we overheat.
  if(b_28segment_bargraph) {
    switch(i_power_level) {
      case 5:
        ms_bargraph_firing.start((i_ramp_interval / 2) - 7); // 13ms per segment
      break;

      case 4:
        ms_bargraph_firing.start((i_ramp_interval / 2) - 3); // 15ms per segment
      break;

      case 3:
        ms_bargraph_firing.start(i_ramp_interval / 2); // 20ms per segment
      break;

      case 2:
        ms_bargraph_firing.start((i_ramp_interval / 2) + 7); // 25ms per segment
      break;

      case 1:
      default:
        ms_bargraph_firing.start((i_ramp_interval / 2) + 12); // 30ms per segment
      break;
    }
  }
}

void cyclotronSpeedUp(uint8_t i_switch) {
  if(i_switch != i_cyclotron_speed_up) {
    if(i_switch == 4) {
      ms_hat_1.start(i_hat_1_delay);
    }

    i_cyclotron_speed_up++;
  }
}

void cyclotronSpeedRevert() {
  i_cyclotron_speed_up = 1;
}

// 2021 mode for optional 28 segment bargraph.
// Checks if we ramp up or down when changing power levels.
// Forces the bargraph to redraw itself to the current power level.
void bargraphPowerCheck2021Alt(bool b_override) {
  if((DEVICE_ACTION_STATUS != ACTION_FIRING && DEVICE_ACTION_STATUS != ACTION_SETTINGS) || b_override) {
    if(i_power_level != i_power_level_prev || b_override) {
      if(i_power_level > i_power_level_prev) {
        b_bargraph_up = true;
      }
      else {
        b_bargraph_up = false;
      }

      switch(i_power_level) {
        case 5:
          ms_bargraph_alt.start(i_bargraph_wait / 3);
        break;

        case 4:
          ms_bargraph_alt.start(i_bargraph_wait / 4);
        break;

        case 3:
          ms_bargraph_alt.start(i_bargraph_wait / 5);
        break;

        case 2:
          ms_bargraph_alt.start(i_bargraph_wait / 6);
        break;

        case 1:
        default:
          ms_bargraph_alt.start(i_bargraph_wait / 7);
        break;
      }
    }
  }
}

void bargraphClearAll() {
  ht_bargraph.clearAll();

  for(uint8_t i = 0; i < i_bargraph_segments; i++) {
    b_bargraph_status[i] = false;
  }
}

void bargraphClearAlt() {
  if(b_28segment_bargraph) {
    bargraphClearAll();

    i_bargraph_status_alt = 0;
  }
}

// This function handles returning all bargraph lookup table values.
uint8_t bargraphLookupTable(uint8_t index) {
  if(b_28segment_bargraph) {
    if(b_bargraph_invert) {
      return PROGMEM_READU8(i_bargraph_invert[index]);
    }
    else {
      return PROGMEM_READU8(i_bargraph_normal[index]);
    }
  }
  return 0;
}

// Draw the bargraph to the current power level instantly.
void bargraphRedraw() {
  if(b_28segment_bargraph) {
    /*
      5: full: 23 - 27  (5 segments)
      4: 3/4: 17 - 22   (6 segments)
      3: 1/2: 12 - 16   (5 segments)
      2: 1/4: 5 - 11    (7 segments)
      1: none: 0 - 4    (5 segments)
    */

    switch(i_power_level) {
      case 1:
      default:
        for(uint8_t i = 0; i < i_bargraph_segments; i++) {
          if(i <= 4) {
            ht_bargraph.setLed(bargraphLookupTable(i));
            b_bargraph_status[i] = true;
          }
          else {
            ht_bargraph.clearLed(bargraphLookupTable(i));
            b_bargraph_status[i] = false;
          }
        }

        ht_bargraph.sendLed(); // Commit the changes.
        i_bargraph_status_alt = 4;
      break;

      case 2:
        for(uint8_t i = 0; i < i_bargraph_segments; i++) {
          if(i <= 11) {
            ht_bargraph.setLed(bargraphLookupTable(i));
            b_bargraph_status[i] = true;
          }
          else {
            ht_bargraph.clearLed(bargraphLookupTable(i));
            b_bargraph_status[i] = false;
          }
        }

        ht_bargraph.sendLed(); // Commit the changes.
        i_bargraph_status_alt = 11;
      break;

      case 3:
        for(uint8_t i = 0; i < i_bargraph_segments; i++) {
          if(i <= 16) {
            ht_bargraph.setLed(bargraphLookupTable(i));
            b_bargraph_status[i] = true;
          }
          else {
            ht_bargraph.clearLed(bargraphLookupTable(i));
            b_bargraph_status[i] = false;
          }
        }

        ht_bargraph.sendLed(); // Commit the changes.
        i_bargraph_status_alt = 16;
      break;

      case 4:
        for(uint8_t i = 0; i < i_bargraph_segments; i++) {
          if(i <= 22) {
            ht_bargraph.setLed(bargraphLookupTable(i));
            b_bargraph_status[i] = true;
          }
          else {
            ht_bargraph.clearLed(bargraphLookupTable(i));
            b_bargraph_status[i] = false;
          }
        }

        ht_bargraph.sendLed(); // Commit the changes.
        i_bargraph_status_alt = 22;
      break;

      case 5:
        for(uint8_t i = 0; i < i_bargraph_segments; i++) {
          ht_bargraph.setLed(bargraphLookupTable(i));
          b_bargraph_status[i] = true;
        }

        ht_bargraph.sendLed(); // Commit the changes.
        i_bargraph_status_alt = 27;
      break;
    }
  }
}

void bargraphPowerCheck() {
  // Control for the 28 segment barmeter bargraph.
  /*
    5: full: 23 - 27  (5 segments)
    4: 3/4: 17 - 22   (6 segments)
    3: 1/2: 12 - 16   (5 segments)
    2: 1/4: 5 - 11    (7 segments)
    1: none: 0 - 4    (5 segments)
  */
  if(b_28segment_bargraph) {
    if(ms_bargraph_alt.justFinished()) {
      uint8_t i_bargraph_multiplier[5] = { 7, 6, 5, 4, 3 };

      if(b_bargraph_up) {
        if(i_bargraph_status_alt < i_bargraph_segments) {
          ht_bargraph.setLedNow(bargraphLookupTable(i_bargraph_status_alt));
          b_bargraph_status[i_bargraph_status_alt] = true;
        }

        switch(i_power_level) {
          case 5:
            if(i_bargraph_status_alt > 27) {
              b_bargraph_up = false;

              i_bargraph_status_alt = 27;

              // A little pause when we reach the top.
              ms_bargraph_alt.start(i_bargraph_wait / 2);
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_level - 1]);
            }
          break;

          case 4:
            if(i_bargraph_status_alt > 21) {
              b_bargraph_up = false;

              // A little pause when we reach the top.
              ms_bargraph_alt.start(i_bargraph_wait / 2);
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_level - 1]);
            }
          break;

          case 3:
            if(i_bargraph_status_alt > 15) {
              b_bargraph_up = false;
              // A little pause when we reach the top.
              ms_bargraph_alt.start(i_bargraph_wait / 2);
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_level - 1]);
            }
          break;

          case 2:
            if(i_bargraph_status_alt > 10) {
              b_bargraph_up = false;
              // A little pause when we reach the top.
              ms_bargraph_alt.start(i_bargraph_wait / 2);
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_level - 1]);
            }
          break;

          case 1:
          default:
            if(i_bargraph_status_alt > 3) {
              b_bargraph_up = false;
              // A little pause when we reach the top.
              ms_bargraph_alt.start(i_bargraph_wait / 2);
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_level - 1]);
            }
          break;
        }

        if(b_bargraph_up) {
          i_bargraph_status_alt++;
        }
      }
      else {
        if(i_bargraph_status_alt < i_bargraph_segments) {
          ht_bargraph.clearLedNow(bargraphLookupTable(i_bargraph_status_alt));
          b_bargraph_status[i_bargraph_status_alt] = false;
        }

        if(i_bargraph_status_alt == 0) {
          b_bargraph_up = true;

          // A little pause when we reach the bottom.
          ms_bargraph_alt.start(i_bargraph_wait / 2);
        }
        else {
          i_bargraph_status_alt--;

          switch(i_power_level) {
            case 5:
              ms_bargraph_alt.start(i_bargraph_interval * 3);
            break;

            case 4:
              ms_bargraph_alt.start(i_bargraph_interval * 4);
            break;

            case 3:
              ms_bargraph_alt.start(i_bargraph_interval * 5);
            break;

            case 2:
              ms_bargraph_alt.start(i_bargraph_interval * 6);
            break;

            case 1:
            default:
              ms_bargraph_alt.start(i_bargraph_interval * 7);
            break;
          }
        }
      }
    }
  }
}

// Fully lights up the bargraph.
void bargraphFull() {
  if(b_28segment_bargraph) {
    for(uint8_t i = 0; i < i_bargraph_segments; i++) {
      ht_bargraph.setLed(bargraphLookupTable(i));
      b_bargraph_status[i] = true;
    }

    ht_bargraph.sendLed(); // Commit the changes.
  }
}

void bargraphRampUp() {
  if(i_vibration_level < i_vibration_level_min) {
    i_vibration_level = i_vibration_level_min;
  }

  if(b_28segment_bargraph) {
    /*
      5: full: 23 - 27 (5 segments)
      4: 3/4: 17 - 22  (6 segments)
      3: 1/2: 12 - 16  (5 segments)
      2: 1/4: 5 - 11   (7 segments)
      1: none: 0 - 4   (5 segments)
    */

    switch(i_bargraph_status_alt) {
      case 0 ... 27:
        ht_bargraph.setLedNow(bargraphLookupTable(i_bargraph_status_alt));
        b_bargraph_status[i_bargraph_status_alt] = true;

        if(i_bargraph_status_alt > 22) {
          vibrationDevice(i_vibration_level + 80);
        }
        else if(i_bargraph_status_alt > 16) {
          vibrationDevice(i_vibration_level + 40);
        }
        else if(i_bargraph_status_alt > 11) {
          vibrationDevice(i_vibration_level + 30);
        }
        else if(i_bargraph_status_alt > 4) {
          vibrationDevice(i_vibration_level + 20);
        }
        else if(i_bargraph_status_alt > 0) {
          vibrationDevice(i_vibration_level + 10);
        }

        i_bargraph_status_alt++;

        if(i_bargraph_status_alt == 28) {
          // A little pause when we reach the top.
          ms_bargraph.start(i_bargraph_wait / 2);
        }
        else {
          ms_bargraph.start(i_bargraph_interval * i_bargraph_multiplier_current);
        }
      break;

      case 28 ... 55:
        uint8_t i_tmp = i_bargraph_status_alt - (i_bargraph_segments - 1);
        i_tmp = i_bargraph_segments - i_tmp;

        ht_bargraph.clearLedNow(bargraphLookupTable(i_tmp));
        b_bargraph_status[i_tmp] = false;

        switch(BARGRAPH_MODE) {
          case BARGRAPH_SUPER_HERO:
          default:
            // Bargraph has ramped up and down. In 1984/1989 mode we want to start the ramping.
            if(i_bargraph_status_alt == 55) {
              ms_bargraph_alt.start(i_bargraph_interval); // Start the alternate bargraph to ramp up and down continuously.
              ms_bargraph.stop();
              b_bargraph_up = true;
              i_bargraph_status_alt = 0;

              vibrationDevice(i_vibration_level);
            }
            else {
              ms_bargraph.start(i_bargraph_interval * i_bargraph_multiplier_current);
              i_bargraph_status_alt++;
            }
          break;
        }
      break;
    }
  }
}

void prepBargraphRampDown() {
  if(DEVICE_STATUS == MODE_ON && DEVICE_ACTION_STATUS == ACTION_IDLE) {
    // If bargraph is set to ramp down during ribbon cable error, we need to set a few things.
    soundIdleLoopStop();

    // Reset some bargraph levels before we ramp the bargraph down.
    i_bargraph_status_alt = i_bargraph_segments; // For 28 segment bargraph

    bargraphFull();

    ms_bargraph.start(d_bargraph_ramp_interval);

    // Prepare to make the bargraph ramp down now.
    bargraphRampUp();
  }
}

void prepBargraphRampUp() {
  if(DEVICE_STATUS == MODE_ON && DEVICE_ACTION_STATUS == ACTION_IDLE) {
    bargraphClearAlt();

    ms_settings_blinking.stop();

    // Prepare a few things before ramping the bargraph back up from a full ramp down.
    bargraphRampUp();
  }
}

void allLightsOff() {
  if(b_28segment_bargraph) {
    bargraphClearAlt();
  }

  // These go LOW to turn off.
  digitalWriteFast(led_slo_blo, LOW);
  digitalWriteFast(led_front_left, LOW); // Turn off the front left LED under the Clippard valve.
  digitalWriteFast(led_hat_1, LOW); // Turn off hat light 1 (not used, but just make sure).
  digitalWriteFast(led_hat_2, LOW); // Turn off hat light 2.

  // These go HIGH to turn off.
  digitalWrite(led_vent, HIGH);
  digitalWriteFast(led_white, HIGH);

  deviceTipOff(); // Not used normally, but make sure it's off.

  // Clear all addressable LEDs by filling the array with black.
  fill_solid(system_leds, CYCLOTRON_LED_COUNT + BARREL_LED_COUNT, CRGB::Black);

  i_bargraph_status = 0;
  i_bargraph_status_alt = 0;

  if(b_power_on_indicator && !ms_power_indicator.isRunning()) {
    ms_power_indicator.start(i_ms_power_indicator);
  }
}

void allLightsOffMenuSystem() {
  // Make sure some of the device lights are off, specifically for the Menu systems.
  digitalWriteFast(led_slo_blo, LOW);
  digitalWrite(led_vent, HIGH);
  digitalWriteFast(led_white, HIGH);
  digitalWriteFast(led_front_left, LOW);

  if(b_power_on_indicator) {
    ms_power_indicator.stop();
    ms_power_indicator_blink.stop();
  }
}

int8_t readRotary() {
  static int8_t rot_enc_table[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};

  prev_next_code <<= 2;

  if(digitalReadFast(r_encoderB)) {
    prev_next_code |= 0x02;
  }

  if(digitalReadFast(r_encoderA)) {
    prev_next_code |= 0x01;
  }

  prev_next_code &= 0x0f;

   // If valid then store as 16 bit data.
   if(rot_enc_table[prev_next_code]) {
      store <<= 4;
      store |= prev_next_code;

      if((store&0xff) == 0x2b) {
        return -1;
      }

      if((store&0xff) == 0x17) {
        return 1;
      }
   }

   return 0;
}

// Top rotary dial on the device.
void checkRotaryEncoder() {
  static int8_t c, val;

  if((val = readRotary())) {
    c += val;
    switch(DEVICE_ACTION_STATUS) {
      case ACTION_CONFIG_EEPROM_MENU:
        // Counter clockwise.
        if(prev_next_code == 0x0b) {
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
                digitalWriteFast(led_slo_blo, HIGH); // Level 2

                // Turn off the other lights.
                digitalWrite(led_vent, HIGH); // Level 3
                digitalWriteFast(led_white, HIGH); // Level 4
                digitalWriteFast(led_front_left, LOW); // Level 5

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
                digitalWriteFast(led_slo_blo, HIGH); // Level 2
                digitalWrite(led_vent, LOW); // Level 3

                // Turn off the other lights.
                digitalWriteFast(led_white, HIGH); // Level 4
                digitalWriteFast(led_front_left, LOW); // Level 5

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
                digitalWriteFast(led_slo_blo, HIGH); // Level 2
                digitalWrite(led_vent, LOW); // Level 3
                digitalWriteFast(led_white, LOW); // Level 4

                // Turn off the other lights.
                digitalWriteFast(led_front_left, LOW); // Level 5

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
                digitalWriteFast(led_slo_blo, HIGH); // Level 2
                digitalWrite(led_vent, LOW); // Level 3
                digitalWriteFast(led_white, LOW); // Level 4
                digitalWriteFast(led_front_left, HIGH); // Level 5

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
        if(prev_next_code == 0x07) {
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
                digitalWriteFast(led_slo_blo, HIGH); // Level 2
                digitalWrite(led_vent, LOW); // Level 3
                digitalWriteFast(led_white, LOW); // Level 4

                // Turn off the other lights.
                digitalWriteFast(led_front_left, LOW); // Level 5

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
                digitalWriteFast(led_slo_blo, HIGH); // Level 2
                digitalWrite(led_vent, LOW); // Level 3

                // Turn off the other lights.
                digitalWriteFast(led_white, HIGH); // Level 4
                digitalWriteFast(led_front_left, LOW); // Level 5

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
                digitalWriteFast(led_slo_blo, HIGH); // Level 2

                // Turn off the other lights.
                digitalWrite(led_vent, HIGH); // Level 3
                digitalWriteFast(led_white, HIGH); // Level 4
                digitalWriteFast(led_front_left, LOW); // Level 5

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
                digitalWriteFast(led_slo_blo, LOW); // Level 2
                digitalWrite(led_vent, HIGH); // Level 3
                digitalWriteFast(led_white, HIGH); // Level 4
                digitalWriteFast(led_front_left, LOW); // Level 5

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
        if(prev_next_code == 0x0b) {
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
                  digitalWriteFast(led_slo_blo, HIGH);

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
        if(prev_next_code == 0x07) {
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
                  digitalWriteFast(led_slo_blo, LOW);

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
            if(prev_next_code == 0x0b) {
              // Decrease the master system volume.
              decreaseVolume();
            }
            else if(prev_next_code == 0x07) {
              // Increase the master system volume.
              increaseVolume();
            }
        }
        else {
          if(DEVICE_ACTION_STATUS == ACTION_FIRING && i_power_level == i_power_level_max) {
            // Do nothing, we are locked in full power level while firing.
          }
          // Counter clockwise.
          else if(prev_next_code == 0x0b) {
            if(switch_device.on() && switch_vent.on() && switch_activate.on()) {
              // Check to see the minimal power level depending on which system mode.
              uint8_t i_tmp_power_level_min = i_power_level_min;

              if(i_power_level - 1 >= i_tmp_power_level_min && DEVICE_STATUS == MODE_ON) {
                i_power_level_prev = i_power_level;
                i_power_level--;

                // Forces a redraw of the bargraph if firing while changing the power level in the BARGRAPH_ANIMATION_ORIGINAL.
                if(b_firing && b_28segment_bargraph && BARGRAPH_FIRING_ANIMATION == BARGRAPH_ANIMATION_ORIGINAL) {
                  bargraphRedraw();
                }

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

          if(DEVICE_ACTION_STATUS == ACTION_FIRING && i_power_level == i_power_level_max) {
            // Do nothing, we are locked in full power level while firing.
          }
          // Clockwise.
          else if(prev_next_code == 0x07) {
            if(switch_device.on() && switch_vent.on() && switch_activate.on()) {
              if(i_power_level + 1 <= i_power_level_max && DEVICE_STATUS == MODE_ON) {
                if(i_power_level + 1 == i_power_level_max && DEVICE_ACTION_STATUS == ACTION_FIRING) {
                  // Do nothing, we do not want to go into max power level if firing in a lower power level already.
                }
                else {
                  i_power_level_prev = i_power_level;
                  i_power_level++;

                  // Forces a redraw of the bargraph if firing while changing the power level if using BARGRAPH_ANIMATION_ORIGINAL.
                  if(b_firing && b_28segment_bargraph && BARGRAPH_FIRING_ANIMATION == BARGRAPH_ANIMATION_ORIGINAL) {
                    bargraphRedraw();
                  }

                  soundIdleLoopStop();
                  soundIdleLoop(false);
                }
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
  if(!ms_bargraph.isRunning() && DEVICE_ACTION_STATUS != ACTION_FIRING) {
    switch(i_power_level) {
      case 1:
      default:
        vibrationDevice(i_vibration_level);
      break;

      case 2:
        vibrationDevice(i_vibration_level + 5);
      break;

      case 3:
        vibrationDevice(i_vibration_level + 10);
      break;

      case 4:
        vibrationDevice(i_vibration_level + 12);
      break;

      case 5:
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
