#include "Header.h"
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

#pragma once

void checkDeviceAction() {

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

  switch(DEVICE_ACTION_STATUS) {
    case ACTION_IDLE:
    default:
      if(DEVICE_STATUS == MODE_ON) {
        if(!ms_cyclotron.isRunning()) {
          // Start the cyclotron animation with consideration for timing from the power level.
          ms_cyclotron.start(getCyclotronDelay());
        }

        switch(POWER_LEVEL) {
          case LEVEL_1:
          default:
            updateCyclotron(C_RED);
          break;
          case LEVEL_2:
            updateCyclotron(C_RED2);
          break;
          case LEVEL_3:
            updateCyclotron(C_RED3);
          break;
          case LEVEL_4:
            updateCyclotron(C_RED4);
          break;
          case LEVEL_5:
            updateCyclotron(C_RED5);
          break;
        }
      }
    break;

    case ACTION_OFF:
      b_device_mash_error = false;
      deviceOff();
      bargraph.off();
    break;

    case ACTION_FIRING:
      if(ms_single_blast.justFinished()) {
        //playEffect(S_FIRE_BLAST, false, i_volume_effects, false, 0, false);

        // Reset the barrel before starting a new pulse.
        barrelLightsOff();

        ms_firing_stream_effects.start(0); // Start new barrel animation.

        switch(POWER_LEVEL) {
          case LEVEL_1:
          default:
            ms_single_blast.start(i_single_blast_delay_level_1);
          break;
          case LEVEL_2:
            ms_single_blast.start(i_single_blast_delay_level_2);
          break;
          case LEVEL_3:
            ms_single_blast.start(i_single_blast_delay_level_3);
          break;
          case LEVEL_4:
            ms_single_blast.start(i_single_blast_delay_level_4);
          break;
          case LEVEL_5:
            ms_single_blast.start(i_single_blast_delay_level_5);
          break;
        }
      }

      if(!b_firing) {
        b_firing = true;
        modeFireStart();
      }

      if(ms_hat_1.isRunning()) {
        if(ms_hat_1.remaining() < i_hat_1_delay / 2) {
          led_Hat2.turnOn();
        }
        else {
          led_Hat2.turnOff();
        }

        if(ms_hat_1.justFinished()) {
          ms_hat_1.start(i_hat_1_delay);
        }
      }

      modeFiring();

      // Stop firing if any of the main switches are turned off or the barrel is retracted.
      if(!switch_vent.on() || !switch_device.on()) {
        modeFireStop();
      }
    break;

    case ACTION_ERROR:
      // No-op, add actions here as needed.
    break;

    case ACTION_ACTIVATE:
      modeActivate();
    break;

    case ACTION_CONFIG_EEPROM_MENU:
      // TODO: Re-introduce custom EEPROM menu options for this device
    break;

    case ACTION_SETTINGS:
      // TODO: Re-introduce standard runtime menu options for this device
    break;
  }

  if(b_firing && DEVICE_ACTION_STATUS != ACTION_FIRING) {
    modeFireStop();
  }

  // Play the firing pulse effect animation.
  if(ms_firing_pulse.justFinished()) {
    firePulseEffect();
  }
}
