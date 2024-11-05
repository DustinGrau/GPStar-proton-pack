/**
 *   GPStar External - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023-2024 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
 *                         & Dustin Grau <dustin.grau@gmail.com>
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

void ledsOff() {
  // Turn off the RGB pins.
  digitalWrite(LED_R_PIN, LOW);
  digitalWrite(LED_G_PIN, LOW);
  digitalWrite(LED_B_PIN, LOW);
}

void blinkLights() {
  if(b_firing) {
    // Only begin blinking if firing.
    if(ms_blink.remaining() < 1) {
      b_blink = !b_blink; // Flip the flag.

      if(i_power > 0) {
        // Speed up the blink with the power level.
        ms_blink.start(i_blink_delay / i_power);
      }
      else {
        // Handle case where power is unset.
        ms_blink.start(i_blink_delay);
      }
    }

    if(b_blink) {
      ledsOff(); // Turn off LED's
    }
    else {
      // Turn on LED's according to the firing mode.
      switch(STREAM_MODE) {
        case PROTON:
          // Red
          digitalWrite(LED_R_PIN, HIGH);
        break;
        case SLIME:
          // Green
          digitalWrite(LED_G_PIN, HIGH);
        break;
        case STASIS:
          // Blue
          digitalWrite(LED_B_PIN, HIGH);
        break;
        case MESON:
          // Orange
          digitalWrite(LED_R_PIN, HIGH);
          digitalWrite(LED_G_PIN, HIGH);
        break;
        default:
          // White
          digitalWrite(LED_R_PIN, HIGH);
          digitalWrite(LED_G_PIN, HIGH);
          digitalWrite(LED_B_PIN, HIGH);
        break;
      }
    }
  }
  else {
    ledsOff(); // Turn off the RGB LED's
    b_blink = true; // Set to the blink (off) state.
  }
}
