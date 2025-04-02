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

void debug(String message) {
  // Writes a debug message to the serial console.
  #if defined(DEBUG_SEND_TO_CONSOLE)
    Serial.println(message); // Print to serial console.
  #endif
  #if defined(DEBUG_SEND_TO_WEBSOCKET)
    ws.textAll(message); // Send a copy to the WebSocket.
  #endif
}

// Obtain a list of partitions for this device.
void printPartitions() {
  const esp_partition_t *partition;
  esp_partition_iterator_t iterator = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

  if (iterator == nullptr) {
    Serial.println(F("No partitions found."));
    return;
  }

  Serial.println(F("Partitions:"));
  while (iterator != nullptr) {
    partition = esp_partition_get(iterator);
    Serial.printf("Label: %s, Size: %lu bytes, Address: 0x%08lx\n",
                  partition->label,
                  partition->size,
                  partition->address);
    iterator = esp_partition_next(iterator);
  }

  esp_partition_iterator_release(iterator);  // Release the iterator once done
}

/*
 * Determine the current state of any LEDs before next FastLED refresh.
 */
void updateLEDs() {
  if(b_ap_started && b_ws_started) {
    // Set the built-in LED to green to indicate the device is fully ready.
    device_leds[0] = getHueAsGRB(0, C_GREEN, 128);
  } else {
    // Set the built-in LED to red while the WiFi and WebSocket are not ready.
    device_leds[0] = getHueAsGRB(0, C_RED, 128);
  }

  if (ms_light.isRunning() && ledcRead(CENTER_LED) == i_min_power) {
    // While the timer is active, keep the center LED lit.
    debug(F("LED On"));
    ledcWrite(CENTER_LED, i_max_power);
  }

  if (ms_light.justFinished()) {
    debug(F("LED Off"));
    ledcWrite(CENTER_LED, i_min_power);
  }

  // For testing, always keep the top LEDs lit.
  ledcWrite(TOP_2WHITE, i_max_power);

  // Set the top LEDs to white.
  for (int i = 0; i < NUM_TOP_PIXELS; i++) {
    top_leds[i] = getHueAsGRB(i, C_WHITE, 255);
  }
}

/*
 * Determine the current state of the blower.
 */
void checkBlower() {
  if (ms_blower.isRunning() && ledcRead(BLOWER_PIN) == i_min_power) {
    // If timer is active but power is not applied, turn on the device AFTER the delay period has elapsed.
    if ((millis() - ms_blower.getStartTime()) >= i_blower_start_delay) {
      debug(F("Blower On"));
      ledcWrite(BLOWER_PIN, i_max_power);
    }
  }

  if (ms_blower.justFinished()) {
    debug(F("Blower Off"));
    ledcWrite(BLOWER_PIN, i_min_power);
  }
}

/*
 * Determine the current state of the smoke device.
 */
void checkSmoke() {
  if (ms_smoke.isRunning() && ledcRead(SMOKE_PIN) == i_min_power) {
    // If timer is active but power is not applied, turn on the device immediately.
    debug(F("Smoke On"));
    ledcWrite(SMOKE_PIN, i_max_power);
  }

  if (ms_smoke.justFinished()) {
    debug(F("Smoke Off"));
    ledcWrite(SMOKE_PIN, i_min_power);
  }
}

/*
 * Perform debounce and get current button/switch states.
 *
 * Required by the ezButton objects.
 */
void switchLoops() {

}

/*
 * Returns true when doors are closed.
 */
bool doorsClosed() {
  return (digitalRead(DOOR_CLOSED_PIN) == 1 && digitalRead(DOOR_OPENED_PIN) == 0);
}

/*
 * Returns true when doors are opened.
 */
bool doorsOpened() {
  return (digitalRead(DOOR_CLOSED_PIN) == 0 && digitalRead(DOOR_OPENED_PIN) == 1);
}

/*
 * Monitor for interactions by user input.
 */
void checkDoors() {
  // Determine whether the trap doors are currently opened or closed.
  if (doorsClosed()) {
    DOOR_STATE = DOORS_CLOSED;
  }
  if (doorsOpened()) {
    DOOR_STATE = DOORS_OPENED;
  }
}

/*
 * Stop a running smoke sequence.
 */
void stopSmoke() {
  // Stop any existing timers before proceeding.
  ms_blower.stop();
  ms_light.stop();
  ms_smoke.stop();

  // Shut down any running devices.
  ledcWrite(BLOWER_PIN, i_min_power);
  ledcWrite(CENTER_LED, i_min_power);
  ledcWrite(SMOKE_PIN, i_min_power);
}

/*
 * Execute a smoke sequence for a given duration.
 */
void startSmoke(uint16_t i_duration) {
  if (!ms_smoke.isRunning()) {
    // Check lower/upper limits for duration.
    if (i_duration < i_smoke_duration_min) {
      i_duration = i_smoke_duration_min;
    }
    if (i_duration > i_smoke_duration_max) {
      i_duration = i_smoke_duration_max;
    }

    // If enabled, begin setting timers for the various devices (LED, blower, and smoke).
    if (b_smoke_enabled && i_duration >= i_smoke_duration_min && i_duration <= i_smoke_duration_max) {
      ms_blower.start(i_duration * 2); // Run the blower twice as long as the smoke duration.
      ms_light.start(i_duration * 1.5); // Keep the LED lit only 1.5x the smoke duration.
      ms_smoke.start(i_duration); // Only run smoke for as long as the system will allow.
    }
  }
}
