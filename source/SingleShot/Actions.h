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
  switch(DEVICE_ACTION_STATUS) {
    case ACTION_IDLE:
    default:
      if(DEVICE_STATUS == MODE_ON) {
        if(!ms_cyclotron.isRunning()) {
          // Start the cyclotron animation with consideration for timing from the power level.
          uint16_t i_dynamic_delay = i_base_cyclotron_delay - ((getPowerLevel() - 1) * (i_base_cyclotron_delay - i_min_cyclotron_delay) / 4);
          ms_cyclotron.start(i_dynamic_delay);
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
      bargraphOff();
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
      // TODO: Re-introduce custom EEPROM menu options for this device
    break;

    case ACTION_SETTINGS:
      // TODO: Re-introduce standard runtime menu options for this device
    break;
  }
}
