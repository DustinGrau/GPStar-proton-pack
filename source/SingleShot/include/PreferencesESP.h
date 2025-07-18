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

#pragma once

/*
 * User Preference Storage/Retrieval via EEPROM
 *
 * This library of functions controls the storing, clearing, reading, and management
 * of user preferences by way of the EEPROM storage area of the ATMega2560 chipset.
 * Values are stored a simple integer types, though they are mapped to boolean and
 * other datatypes as necessary during the reading/storing actions. Additionally,
 * a user may not even be using the EEPROM storage area and thus defaults may be
 * set when values are not present.
 */

/*
 * Function prototypes.
 */
void readEEPROM();
void saveConfigEEPROM();
void loadConfigEEPROM();
void clearConfigEEPROM();
void updateCRCEEPROM(uint32_t);
uint32_t getCRCEEPROM(void);
uint32_t eepromCRC(void);
void resetOverheatLevels();
void resetWhiteLEDBlinkRate();

// Include ESP32 Preferences library
#include <Preferences.h>

// Preferences for system configuration, which will use a "led" and "config" namespaces.
// For Wireless.h will store SSID and AP password within a "credentials" namespace.
Preferences preferences;

// Data structure for configuration settings (stored in Preferences)
struct objConfigEEPROM {
  uint8_t deviceBootErrors;
  uint8_t ventLightAutoIntensity;
  uint8_t invertBargraph;
  uint8_t defaultSystemVolume;
  uint8_t deviceVibration;
} gObjConfigEEPROM;

// Save configuration preferences to NVS (ESP32)
void saveConfigEEPROM() {
  // Convert the current EEPROM volume value into a percentage.
  uint8_t eepromVolumeMasterPercentage = 100 * (MINIMUM_VOLUME - i_volume_master_eeprom) / MINIMUM_VOLUME;

  // 1 = false, 2 = true.
  gObjConfigEEPROM.deviceBootErrors = b_device_boot_errors ? 2 : 1;
  gObjConfigEEPROM.ventLightAutoIntensity = b_vent_light_control ? 2 : 1;
  gObjConfigEEPROM.invertBargraph = b_bargraph_invert ? 2 : 1;
  gObjConfigEEPROM.defaultSystemVolume = (eepromVolumeMasterPercentage <= 100) ? (eepromVolumeMasterPercentage + 1) : 101;

  switch (VIBRATION_MODE_EEPROM) {
    case VIBRATION_ALWAYS:
    default:
      gObjConfigEEPROM.deviceVibration = 1;
    break;
    case VIBRATION_FIRING_ONLY:
      gObjConfigEEPROM.deviceVibration = 2;
    break;
    case VIBRATION_NONE:
      gObjConfigEEPROM.deviceVibration = 3;
    break;
  }

  if (preferences.begin("config", false)) {
    preferences.putBytes("config", &gObjConfigEEPROM, sizeof(gObjConfigEEPROM));
    preferences.end();
  }

  updateCRCEEPROM(eepromCRC());
}

// Load configuration preferences from NVS (ESP32)
void loadConfigEEPROM() {
  if (preferences.begin("config", true)) {
    if (preferences.isKey("config")) {
      preferences.getBytes("config", &gObjConfigEEPROM, sizeof(gObjConfigEEPROM));
    }
    preferences.end();
  }
}

// Clear configuration preferences in NVS (ESP32)
void clearConfigEEPROM() {
  if (preferences.begin("config", false)) {
    preferences.clear();
    preferences.end();
  }

  updateCRCEEPROM(eepromCRC());
}

// CRC helpers for Preferences (ESP32)
void updateCRCEEPROM(uint32_t crc) {
  if (preferences.begin("crc", false)) {
    preferences.putUInt("crc", crc);
    preferences.end();
  }
}

uint32_t getCRCEEPROM() {
  uint32_t crc = 0;

  if (preferences.begin("crc", true)) {
    crc = preferences.getUInt("crc");
    preferences.end();
  }

  return crc;
}

// Calculate CRC for all stored preferences (ESP32)
uint32_t eepromCRC() {
  CRC32 crc;
  loadConfigEEPROM();
  crc.update((uint8_t*)&gObjConfigEEPROM, sizeof(gObjConfigEEPROM));
  return crc.finalize();
}

// Read all user preferences from Preferences (ESP32)
void readEEPROM() {
  uint32_t storedCrc = getCRCEEPROM();
  uint32_t calcCrc = eepromCRC();
  if (storedCrc == calcCrc) {
    // Map loaded config to runtime variables
    b_device_boot_errors = (gObjConfigEEPROM.deviceBootErrors > 1);
    b_vent_light_control = (gObjConfigEEPROM.ventLightAutoIntensity > 1);
    b_bargraph_invert = (gObjConfigEEPROM.invertBargraph > 1);

    if (gObjConfigEEPROM.defaultSystemVolume > 0 && gObjConfigEEPROM.defaultSystemVolume < 102) {
      i_volume_master_percentage = gObjConfigEEPROM.defaultSystemVolume - 1;
      i_volume_master_eeprom = MINIMUM_VOLUME - ((MINIMUM_VOLUME - i_volume_abs_max) * i_volume_master_percentage / 100);
      i_volume_revert = i_volume_master_eeprom;
      i_volume_master = i_volume_master_eeprom;
    }

    switch (gObjConfigEEPROM.deviceVibration) {
      case 3:
        VIBRATION_MODE_EEPROM = VIBRATION_NONE;
        break;
      case 2:
        VIBRATION_MODE_EEPROM = VIBRATION_FIRING_ONLY;
        break;
      case 1:
      default:
        VIBRATION_MODE_EEPROM = VIBRATION_ALWAYS;
        break;
    }
    VIBRATION_MODE = VIBRATION_MODE_EEPROM;
  } else {
    // CRC mismatch: clear preferences and notify
    playEffect(S_VOICE_EEPROM_LOADING_FAILED_RESET);
    clearConfigEEPROM();
  }
}
