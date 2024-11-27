/**
 *   GPStar Stream Effects - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2024 Dustin Grau <dustin.grau@gmail.com>
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
  fill_solid(device_leds, DEVICE_NUM_LEDS, CRGB::Black);
}

// Animate the lights with the current palette
void animateLights() {
  static uint16_t primaryWavePosition = 0;
  static uint16_t secondaryWavePosition = 0;

  if (ms_anim_change.justFinished()) {
    // Scale animation duration to maintain flexibility for i_power range
    uint8_t adjustedPower = 6 - i_power; // Reverse the scale for timings
    uint16_t i_timer = i_animation_duration / (adjustedPower * 2);
    ms_anim_change.start(i_timer);

    for (int i = 0; i < DEVICE_NUM_LEDS; i++) {
      // Calculate brightness for primary wave
      uint8_t brightness = map(sin8((primaryWavePosition + i * 16) % 255), 0, 255, i_min_brightness, i_max_brightness);
      CRGB color = ColorFromPalette(cp_StreamPalette, 128);  // Use primary color
      device_leds[i] = color;
      device_leds[i].fadeToBlackBy(255 - brightness);

      // Add secondary color at full brightness moving twice as fast
      if ((secondaryWavePosition + i * 32) % 255 < 16) {  // Narrow band of secondary color
        device_leds[i] = ColorFromPalette(cp_StreamPalette, 0);  // Use secondary color
      }
    }

    // Update wave positions
    primaryWavePosition += i_animation_step;
    secondaryWavePosition += i_animation_step * 2;  // Secondary wave moves 2x faster
  }
}
