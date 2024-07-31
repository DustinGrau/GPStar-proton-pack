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
  switch(DEVICE_ACTION_STATUS) {
    case ACTION_IDLE:
    default:
      if(DEVICE_STATUS == MODE_ON) {
        // No-op, add actions here as needed.
      }
    break;

    case ACTION_OFF:
      b_device_mash_error = false;
      deviceOff();
    break;

    case ACTION_FIRING:
      if(ms_single_blast.justFinished()) {
        playEffect(S_FIRE_BLAST, false, i_volume_effects, false, 0, false);

        // Reset the barrel before starting a new pulse.
        barrelLightsOff();

        ms_firing_stream_effects.start(0); // Start new barrel animation.

        switch(i_power_level) {
          case 5:
            ms_single_blast.start(i_single_blast_delay_level_5);
          break;

          case 4:
            ms_single_blast.start(i_single_blast_delay_level_4);
          break;

          case 3:
            ms_single_blast.start(i_single_blast_delay_level_3);
          break;

          case 2:
            ms_single_blast.start(i_single_blast_delay_level_2);
          break;

          case 1:
          default:
            ms_single_blast.start(i_single_blast_delay_level_1);
          break;
        }
      }

      if(!b_firing) {
        b_firing = true;
        modeFireStart();
      }

      if(ms_hat_1.isRunning()) {
        if(ms_hat_1.remaining() < i_hat_1_delay / 2) {
          digitalWriteFast(led_hat_2, HIGH);
        }
        else {
          digitalWriteFast(led_hat_2, LOW);
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
      settingsBlinkingLights();

      // TODO: Re-introduce custom EEPROM menu options for this device
    break;

    case ACTION_SETTINGS:
      settingsBlinkingLights();

      // TODO: Re-introduce standard runtime menu options for this device
    break;
  }
}
