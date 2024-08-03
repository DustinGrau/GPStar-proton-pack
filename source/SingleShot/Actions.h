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

// Forward function declarations
void gripButtonCheck();

void checkDeviceAction() {
  switch(DEVICE_STATUS) {
    case MODE_OFF:
      // Reset the count of the device switch
      if(!switch_intensify.on()) {
        deviceSwitchedCount = 0;
        ventSwitchedCount = 0;
      }
      else if(DEVICE_ACTION_STATUS != ACTION_SETTINGS && DEVICE_ACTION_STATUS != ACTION_CONFIG_EEPROM_MENU && switch_intensify.on() && ventSwitchedCount >= 5) {
        stopEffect(S_BEEPS);
        playEffect(S_BEEPS);

        stopEffect(S_VOICE_EEPROM_CONFIG_MENU);
        playEffect(S_VOICE_EEPROM_CONFIG_MENU);

        DEVICE_ACTION_STATUS = ACTION_CONFIG_EEPROM_MENU;

        // Update lights and context for menu.
        deviceEnterMenu();

        ms_settings_blinking.start(i_settings_blinking_delay);
      }

      if(switch_activate.on() && DEVICE_ACTION_STATUS == ACTION_IDLE) {
        // Activate the device if previously idle.
        DEVICE_ACTION_STATUS = ACTION_ACTIVATE;
      }

      if(DEVICE_ACTION_STATUS != ACTION_CONFIG_EEPROM_MENU) {
        if(switch_grip.pushed()) {
          if(DEVICE_ACTION_STATUS != ACTION_SETTINGS) {
            DEVICE_ACTION_STATUS = ACTION_SETTINGS;

            // Update lights and context for menu.
            deviceEnterMenu();

            ms_settings_blinking.start(i_settings_blinking_delay);
          }
          else {
            // Only exit the settings menu when on menu level 1 at option level 5 (very top of the menu chain)
            if(DEVICE_ACTION_STATUS == ACTION_SETTINGS && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && MENU_OPTION_LEVEL == OPTION_5) {
              deviceExitMenu();
            }
          }
        }
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

      // If the activate is switched off during error mode, reset the device.
      if(!switch_activate.on()) {
        b_device_boot_error_on = false;
        deviceOff();
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

      // Determine if the special grip button has been pressed (eg. menu operation);
      gripButtonCheck();

      // Determine the light status on the device and any beeps.
      deviceLightControlCheck();

      // Check if we should fire, or if the device was turned off.
      fireControlCheck();
    break;
  }

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

    case ACTION_ACTIVATE:
      modeActivate();
    break;

    case ACTION_FIRING:
      if(ms_single_blast.justFinished()) {
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

    case ACTION_CONFIG_EEPROM_MENU:
      // TODO: Re-introduce Config EEPROM menu options for this device
    break;

    case ACTION_SETTINGS:
      // TODO: Re-introduce standard runtime menu options for this device
    break;
  }

  if(b_firing && DEVICE_ACTION_STATUS != ACTION_FIRING) {
    // User is firing but we've switched into an action that is not firing.
    modeFireStop();
  }
}

// Check the state of the grip button to determine whether we have entered the settings menu.
void gripButtonCheck() {
  // Proceed if device is in an idle state or already in the settings menu.
  if(DEVICE_ACTION_STATUS == ACTION_IDLE || DEVICE_ACTION_STATUS == ACTION_SETTINGS) {
    if(switch_grip.pushed() && !(switch_device.on() && switch_vent.on())) {
      // Switch between firing mode and settings mode.
      if(DEVICE_ACTION_STATUS != ACTION_SETTINGS) {
        // Not currently in settings system.
        DEVICE_ACTION_STATUS = ACTION_SETTINGS;
        ms_settings_blinking.start(i_settings_blinking_delay);
        deviceEnterMenu();
      }
      else if(DEVICE_MENU_LEVEL == MENU_LEVEL_1 && MENU_OPTION_LEVEL == OPTION_5) {
        // Only exit the settings when at option #5 on menu level 1.
        DEVICE_ACTION_STATUS = ACTION_IDLE;
        ms_settings_blinking.stop();
        deviceExitMenu();
      }
    }
    else if(DEVICE_ACTION_STATUS == ACTION_SETTINGS && (switch_vent.on() || switch_device.on())) {
      // Exit the settings menu if the user turns the device switches back on.
      DEVICE_ACTION_STATUS = ACTION_IDLE;
      ms_settings_blinking.stop();
      bargraph.clear();
      deviceExitMenu();
    }
  }
}

void encoderChangedMenuOption() {
  // Handle menu navigation based on rotation of the encoder
  if(encoder.STATE == ENCODER_CW) {
    if(decreaseOptionLevel()) {
      bargraph.showBars(MENU_OPTION_LEVEL);
    }
  }
  else if(encoder.STATE == ENCODER_CCW) {
    if(increaseOptionLevel()) {
      bargraph.showBars(MENU_OPTION_LEVEL);
    }
  }
}

// Performs an action directly related to input actions via the encoder.
void checkEncoderAction() {
  if(encoder.STATE == ENCODER_IDLE) {
    return; // Leave if no change has occurred.
  }

  switch(DEVICE_STATUS) {
    case MODE_OFF:
      if(b_playing_music) {
        if(encoder.STATE == ENCODER_CW) {
            // Increase the music volume.
            increaseVolumeMusic();
        }
        else if(encoder.STATE == ENCODER_CCW) {
            // Decrease the music volume.
            decreaseVolumeMusic();
        }
      }

      switch(DEVICE_ACTION_STATUS) {
        case ACTION_IDLE:
        case ACTION_OFF:
        case ACTION_ACTIVATE:
        case ACTION_FIRING:
        case ACTION_ERROR:
          // No-Op.
        break;

        case ACTION_SETTINGS:
          // Perform menu and option navigation.
          encoderChangedMenuOption();
        break;

        case ACTION_CONFIG_EEPROM_MENU:
          // No-Op, for now
        break;
      }
    break; // MODE_OFF

    case MODE_ERROR:
      // No-Op. No actions to take when in error mode.
    break; // MODE_ERROR

    case MODE_ON:
      if(DEVICE_ACTION_STATUS == ACTION_SETTINGS && !switch_intensify.on() && !switch_vent.on() && !switch_device.on()) {
        // Perform menu and option navigation.
        encoderChangedMenuOption();
      }

      if(DEVICE_ACTION_STATUS == ACTION_IDLE && !switch_intensify.on() && !switch_vent.on() && !switch_device.on()) {
        if(encoder.STATE == ENCODER_CW) {
          // Increase the master system volume.
          increaseVolume();
        }
        else if(encoder.STATE == ENCODER_CCW) {
          // Decrease the master system volume.
          decreaseVolume();
        }
      }

      if(DEVICE_ACTION_STATUS == ACTION_IDLE && switch_device.on() && switch_vent.on() && switch_activate.on()) {
        if(encoder.STATE == ENCODER_CW) {
          if(increasePowerLevel()) {
            soundIdleLoopStop();
            soundIdleLoop(false);
          }
        }
        else if(encoder.STATE == ENCODER_CCW) {
          if(decreasePowerLevel()) {
            soundIdleLoopStop();
            soundIdleLoop(false);
          }
        }
      }
    break; // MODE_ON
  }

  // switch(DEVICE_ACTION_STATUS) {
  //   case ACTION_CONFIG_EEPROM_MENU:
  //     // Counter clockwise.
  //     if(encoder.STATE == ENCODER_CCW) {
  //       if(DEVICE_MENU_LEVEL == MENU_LEVEL_3 && MENU_OPTION_LEVEL == OPTION_5 && switch_intensify.on() && !switch_grip.on()) {
  //         // Adjust the volume manually
  //         decreaseVolumeEEPROM();
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_5 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_4 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_3 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_2 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_1 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_5 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_4 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_3 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_2 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_1 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(MENU_OPTION_LEVEL < 1) {
  //         switch(DEVICE_MENU_LEVEL) {
  //           case MENU_LEVEL_1:
  //             DEVICE_MENU_LEVEL = MENU_LEVEL_2;
  //             MENU_OPTION_LEVEL = OPTION_5;

  //             // Turn on some lights to visually indicate which menu we are in.
  //             led_SloBlo.turnOn(); // Level 2

  //             // Turn off the other lights.
  //             led_Vent.turnOff(); // Level 3
  //             led_TopWhite.turnOff(); // Level 4
  //             led_Clippard.turnOff(); // Level 5

  //             // Play an indication beep to notify we have changed menu levels.
  //             stopEffect(S_BEEPS);
  //             playEffect(S_BEEPS);

  //             stopEffect(S_VOICE_LEVEL_1);
  //             stopEffect(S_VOICE_LEVEL_2);
  //             stopEffect(S_VOICE_LEVEL_3);
  //             stopEffect(S_VOICE_LEVEL_4);
  //             stopEffect(S_VOICE_LEVEL_5);

  //             playEffect(S_VOICE_LEVEL_2);
  //           break;

  //           case MENU_LEVEL_2:
  //             DEVICE_MENU_LEVEL = MENU_LEVEL_3;
  //             MENU_OPTION_LEVEL = OPTION_5;

  //             // Turn on some lights to visually indicate which menu we are in.
  //             led_SloBlo.turnOn(); // Level 2
  //             led_Vent.turnOn(); // Level 3

  //             // Turn off the other lights.
  //             led_TopWhite.turnOff(); // Level 4
  //             led_Clippard.turnOff(); // Level 5

  //             // Play an indication beep to notify we have changed menu levels.
  //             stopEffect(S_BEEPS);
  //             playEffect(S_BEEPS);

  //             stopEffect(S_VOICE_LEVEL_1);
  //             stopEffect(S_VOICE_LEVEL_2);
  //             stopEffect(S_VOICE_LEVEL_3);
  //             stopEffect(S_VOICE_LEVEL_4);
  //             stopEffect(S_VOICE_LEVEL_5);

  //             playEffect(S_VOICE_LEVEL_3);
  //           break;

  //           case MENU_LEVEL_3:
  //             DEVICE_MENU_LEVEL = MENU_LEVEL_4;
  //             MENU_OPTION_LEVEL = OPTION_5;

  //             // Turn on some lights to visually indicate which menu we are in.
  //             led_SloBlo.turnOn(); // Level 2
  //             led_Vent.turnOn(); // Level 3
  //             led_TopWhite.turnOn(); // Level 4

  //             // Turn off the other lights.
  //             led_Clippard.turnOff(); // Level 5

  //             // Play an indication beep to notify we have changed menu levels.
  //             stopEffect(S_BEEPS);
  //             playEffect(S_BEEPS);

  //             stopEffect(S_VOICE_LEVEL_1);
  //             stopEffect(S_VOICE_LEVEL_2);
  //             stopEffect(S_VOICE_LEVEL_3);
  //             stopEffect(S_VOICE_LEVEL_4);
  //             stopEffect(S_VOICE_LEVEL_5);

  //             playEffect(S_VOICE_LEVEL_4);
  //           break;

  //           case MENU_LEVEL_4:
  //             DEVICE_MENU_LEVEL = MENU_LEVEL_5;
  //             MENU_OPTION_LEVEL = OPTION_5;

  //             // Turn on some lights to visually indicate which menu we are in.
  //             led_SloBlo.turnOn(); // Level 2
  //             led_Vent.turnOn(); // Level 3
  //             led_TopWhite.turnOn(); // Level 4
  //             led_Clippard.turnOn(); // Level 5

  //             // Play an indication beep to notify we have changed menu levels.
  //             stopEffect(S_BEEPS);
  //             playEffect(S_BEEPS);

  //             stopEffect(S_VOICE_LEVEL_1);
  //             stopEffect(S_VOICE_LEVEL_2);
  //             stopEffect(S_VOICE_LEVEL_3);
  //             stopEffect(S_VOICE_LEVEL_4);
  //             stopEffect(S_VOICE_LEVEL_5);

  //             playEffect(S_VOICE_LEVEL_5);
  //           break;

  //           // Menu 5 the deepest level.
  //           case MENU_LEVEL_5:
  //           default:
  //             MENU_OPTION_LEVEL = OPTION_1;
  //           break;
  //         }
  //       }
  //       else {
  //         decreaseMenuLevel();
  //       }
  //     }

  //     // Clockwise.
  //     if(encoder.STATE == ENCODER_CW) {
  //       if(DEVICE_MENU_LEVEL == MENU_LEVEL_3 && MENU_OPTION_LEVEL == OPTION_5 && switch_intensify.on() && !switch_grip.on()) {
  //         // Adjust the volume manually
  //         increaseVolumeEEPROM();
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_5 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_4 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_3 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_2 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_1 && switch_intensify.on() && !switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_5 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_4 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_3 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_2 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(DEVICE_MENU_LEVEL == MENU_LEVEL_4 && MENU_OPTION_LEVEL == OPTION_1 && !switch_intensify.on() && switch_grip.on()) {
  //       }
  //       else if(MENU_OPTION_LEVEL > 4) {
  //         switch(DEVICE_MENU_LEVEL) {
  //           case MENU_LEVEL_5:
  //             DEVICE_MENU_LEVEL = MENU_LEVEL_4;
  //             MENU_OPTION_LEVEL = OPTION_5;

  //             // Turn on some lights to visually indicate which menu we are in.
  //             led_SloBlo.turnOn(); // Level 2
  //             led_Vent.turnOn(); // Level 3
  //             led_TopWhite.turnOn(); // Level 4

  //             // Turn off the other lights.
  //             led_Clippard.turnOff(); // Level 5

  //             // Play an indication beep to notify we have changed menu levels.
  //             stopEffect(S_BEEPS);
  //             playEffect(S_BEEPS);

  //             stopEffect(S_VOICE_LEVEL_1);
  //             stopEffect(S_VOICE_LEVEL_2);
  //             stopEffect(S_VOICE_LEVEL_3);
  //             stopEffect(S_VOICE_LEVEL_4);
  //             stopEffect(S_VOICE_LEVEL_5);

  //             playEffect(S_VOICE_LEVEL_4);
  //           break;

  //           case MENU_LEVEL_4:
  //             DEVICE_MENU_LEVEL = MENU_LEVEL_3;
  //             MENU_OPTION_LEVEL = OPTION_5;

  //             // Turn on some lights to visually indicate which menu we are in.
  //             led_SloBlo.turnOn(); // Level 2
  //             led_Vent.turnOn(); // Level 3

  //             // Turn off the other lights.
  //             led_TopWhite.turnOff(); // Level 4
  //             led_Clippard.turnOff(); // Level 5

  //             // Play an indication beep to notify we have changed menu levels.
  //             stopEffect(S_BEEPS);
  //             playEffect(S_BEEPS);

  //             stopEffect(S_VOICE_LEVEL_1);
  //             stopEffect(S_VOICE_LEVEL_2);
  //             stopEffect(S_VOICE_LEVEL_3);
  //             stopEffect(S_VOICE_LEVEL_4);
  //             stopEffect(S_VOICE_LEVEL_5);

  //             playEffect(S_VOICE_LEVEL_3);
  //           break;

  //           case MENU_LEVEL_3:
  //             DEVICE_MENU_LEVEL = MENU_LEVEL_2;
  //             MENU_OPTION_LEVEL = OPTION_5;

  //             // Turn on some lights to visually indicate which menu we are in.
  //             led_SloBlo.turnOn(); // Level 2

  //             // Turn off the other lights.
  //             led_Vent.turnOff(); // Level 3
  //             led_TopWhite.turnOff(); // Level 4
  //             led_Clippard.turnOff(); // Level 5

  //             // Play an indication beep to notify we have changed menu levels.
  //             stopEffect(S_BEEPS);
  //             playEffect(S_BEEPS);

  //             stopEffect(S_VOICE_LEVEL_1);
  //             stopEffect(S_VOICE_LEVEL_2);
  //             stopEffect(S_VOICE_LEVEL_3);
  //             stopEffect(S_VOICE_LEVEL_4);
  //             stopEffect(S_VOICE_LEVEL_5);

  //             playEffect(S_VOICE_LEVEL_2);
  //           break;

  //           case MENU_LEVEL_2:
  //             DEVICE_MENU_LEVEL = MENU_LEVEL_1;
  //             MENU_OPTION_LEVEL = OPTION_5;

  //             // Turn off the other lights.
  //             led_SloBlo.turnOff(); // Level 2
  //             led_Vent.turnOff(); // Level 3
  //             led_TopWhite.turnOff(); // Level 4
  //             led_Clippard.turnOff(); // Level 5

  //             // Play an indication beep to notify we have changed menu levels.
  //             stopEffect(S_BEEPS);
  //             playEffect(S_BEEPS);

  //             stopEffect(S_VOICE_LEVEL_1);
  //             stopEffect(S_VOICE_LEVEL_2);
  //             stopEffect(S_VOICE_LEVEL_3);
  //             stopEffect(S_VOICE_LEVEL_4);
  //             stopEffect(S_VOICE_LEVEL_5);

  //             playEffect(S_VOICE_LEVEL_1);
  //           break;

  //           case MENU_LEVEL_1:
  //           default:
  //             // Cannot go any further than menu level 1.
  //             MENU_OPTION_LEVEL = OPTION_5;
  //           break;
  //         }
  //       }
  //       else {
  //         increaseMenuLevel();
  //       }
  //     }
  //   break;

  //   case ACTION_SETTINGS:
  //     // Counter clockwise.
  //     if(encoder.STATE == ENCODER_CCW) {
  //       if(MENU_OPTION_LEVEL == OPTION_4 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && switch_intensify.on() && !switch_grip.on()) {
  //         // No-op
  //       }
  //       else if(MENU_OPTION_LEVEL == OPTION_3 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && switch_intensify.on() && !switch_grip.on()) {
  //         // Lower the sound effects volume.
  //         decreaseVolumeEffects();
  //       }
  //       else if(MENU_OPTION_LEVEL == OPTION_3 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && !switch_intensify.on() && switch_grip.on() && b_playing_music) {
  //         // Decrease the music volume.
  //         decreaseVolumeMusic();
  //       }
  //       else if(MENU_OPTION_LEVEL < 1) {
  //         // We are entering the sub menu. Only accessible when the Single-Shot Blaster is powered down.
  //         if(DEVICE_STATUS == MODE_OFF) {
  //           switch(DEVICE_MENU_LEVEL) {
  //             case MENU_LEVEL_1:
  //               DEVICE_MENU_LEVEL = MENU_LEVEL_2;
  //               MENU_OPTION_LEVEL = OPTION_5;

  //               // Turn on the slo blow led to indicate we are in the Single-Shot Blaster sub menu.
  //               led_SloBlo.turnOn();

  //               // Play an indication beep to notify we have changed menu levels.
  //               stopEffect(S_BEEPS);
  //               playEffect(S_BEEPS);

  //               stopEffect(S_VOICE_LEVEL_1);
  //               stopEffect(S_VOICE_LEVEL_2);
  //               stopEffect(S_VOICE_LEVEL_3);
  //               stopEffect(S_VOICE_LEVEL_4);
  //               stopEffect(S_VOICE_LEVEL_5);

  //               playEffect(S_VOICE_LEVEL_2);
  //             break;

  //             case MENU_LEVEL_2:
  //             default:
  //               // Cannot go further than level 2 for this menu.
  //               MENU_OPTION_LEVEL = OPTION_1;
  //             break;
  //           }
  //         }
  //         else {
  //           MENU_OPTION_LEVEL = OPTION_1;
  //         }
  //       }
  //       else {
  //         decreaseOptionLevel();
  //       }
  //     }

  //     // Clockwise.
  //     if(encoder.STATE == ENCODER_CW) {
  //       if(MENU_OPTION_LEVEL == OPTION_4 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && switch_intensify.on() && !switch_grip.on()) {
  //         // No-op
  //       }
  //       else if(MENU_OPTION_LEVEL == OPTION_3 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && switch_intensify.on() && !switch_grip.on()) {
  //         // Increase sound effects volume.
  //         increaseVolumeEffects();
  //       }
  //       else if(MENU_OPTION_LEVEL == OPTION_3 && DEVICE_MENU_LEVEL == MENU_LEVEL_1 && !switch_intensify.on() && switch_grip.on() && b_playing_music) {
  //         // Increase music volume.
  //         increaseVolumeMusic();
  //       }
  //       else if(MENU_OPTION_LEVEL > 4) {
  //         // We are leaving changing menu levels. Only accessible when the Single-Shot Blaster is powered down.
  //         if(DEVICE_STATUS == MODE_OFF) {
  //           switch(DEVICE_MENU_LEVEL) {
  //             case MENU_LEVEL_2:
  //               DEVICE_MENU_LEVEL = MENU_LEVEL_1;
  //               MENU_OPTION_LEVEL = OPTION_1;

  //               // Turn off the slo blow led to indicate we are no longer in the Single-Shot Blaster sub menu.
  //               led_SloBlo.turnOff();

  //               // Play an indication beep to notify we have changed menu levels.
  //               stopEffect(S_BEEPS);
  //               playEffect(S_BEEPS);

  //               stopEffect(S_VOICE_LEVEL_1);
  //               stopEffect(S_VOICE_LEVEL_2);
  //               stopEffect(S_VOICE_LEVEL_3);
  //               stopEffect(S_VOICE_LEVEL_4);
  //               stopEffect(S_VOICE_LEVEL_5);

  //               playEffect(S_VOICE_LEVEL_1);
  //             break;

  //             case MENU_LEVEL_1:
  //             default:
  //               // Level 1 is the first menu and nothing above it.
  //               MENU_OPTION_LEVEL = OPTION_5;
  //             break;
  //           }
  //         }
  //         else {
  //           MENU_OPTION_LEVEL = OPTION_5;
  //         }
  //       }
  //       else {
  //         decreaseOptionLevel();
  //       }
  //     }
  //   break;

  //   default:
  //     if(DEVICE_STATUS == MODE_ON && switch_intensify.on() && !switch_vent.on() && !switch_device.on()) {
  //         // Counter clockwise.
  //         if(encoder.STATE == ENCODER_CCW) {
  //           // Decrease the master system volume.
  //           decreaseVolume();
  //         }
  //         else if(encoder.STATE == ENCODER_CW) {
  //           // Increase the master system volume.
  //           increaseVolume();
  //         }
  //     }
  //     else {
  //       if(DEVICE_ACTION_STATUS == ACTION_FIRING && POWER_LEVEL == LEVEL_5) {
  //         // Do nothing, we are locked in full power level while firing.
  //       }
  //       // Counter clockwise.
  //       else if(encoder.STATE == ENCODER_CCW) {
  //         if(switch_device.on() && switch_vent.on() && switch_activate.on()) {
  //           // Check to see the minimal power level depending on which system mode.
  //           if(decreasePowerLevel() && DEVICE_STATUS == MODE_ON) {
  //             soundIdleLoopStop();
  //             soundIdleLoop(false);
  //           }
  //         }
  //         else if(!switch_device.on() && switch_vent.on() && ms_firing_mode_switch.remaining() < 1 && DEVICE_STATUS == MODE_ON) {
  //           // Counter clockwise firing mode selection.
  //           STREAM_MODE = PROTON;
  //           ms_firing_mode_switch.start(i_firing_mode_switch_delay);
  //         }

  //         // Decrease the music volume if the device is off. A quick easy way to adjust the music volume on the go.
  //         if(DEVICE_STATUS == MODE_OFF && b_playing_music && !switch_intensify.on()) {
  //           decreaseVolumeMusic();
  //         }
  //         else if(DEVICE_STATUS == MODE_OFF && switch_intensify.on()) {
  //           // Decrease the master volume of the device.
  //           decreaseVolume();
  //         }
  //       }

  //       if(DEVICE_ACTION_STATUS == ACTION_FIRING && POWER_LEVEL == LEVEL_5) {
  //         // Do nothing, we are locked in full power level while firing.
  //       }
  //       // Clockwise.
  //       else if(encoder.STATE == ENCODER_CW) {
  //         if(switch_device.on() && switch_vent.on() && switch_activate.on()) {
  //           if(increasePowerLevel() && DEVICE_STATUS == MODE_ON) {
  //             soundIdleLoopStop();
  //             soundIdleLoop(false);
  //           }
  //         }
  //         else if(!switch_device.on() && switch_vent.on() && ms_firing_mode_switch.remaining() < 1 && DEVICE_STATUS == MODE_ON) {
  //           ms_firing_mode_switch.start(i_firing_mode_switch_delay);
  //         }

  //         // Increase the music volume if the device is off. A quick easy way to adjust the music volume on the go.
  //         if(DEVICE_STATUS == MODE_OFF && b_playing_music && !switch_intensify.on()) {
  //           increaseVolumeMusic();
  //         }
  //         else if(DEVICE_STATUS == MODE_OFF && switch_intensify.on()) {
  //           // Increase the master volume of the Single-Shot Blaster only.
  //           increaseVolume();
  //         }
  //       }
  //     }
  //   break;
  // }
}