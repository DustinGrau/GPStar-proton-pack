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
  static uint16_t i_led_position = 0;
  static uint16_t i_secondary_position = 0; // For the secondary color

  // Ensure power level stays within the range 1-5
  i_power = constrain(i_power, 1, 5);

  // Map power level (1-5) to animation duration (50-10ms)
  uint16_t i_timer = map(i_power, 1, 5, 50, 10);
  ms_anim_change.start(i_timer);

  if (ms_anim_change.justFinished()) {
    for (int i = 0; i < DEVICE_NUM_LEDS; i++) {
      // Primary color wave (brightness animation)
      uint8_t brightness = map(
        sin8((i_led_position + i * 32) % 255), 
        0, 255, 
        i_min_brightness, i_max_brightness
      );
      CRGB primaryColor = ColorFromPalette(cp_StreamPalette, i_led_position + i * 32);
      device_leds[i] = primaryColor.nscale8(brightness);

      // Secondary color wave (fixed brightness)
      if (((i_secondary_position / 2) + i * 64) % 255 < 10) { // Secondary travels 2x speed
        CRGB secondaryColor = ColorFromPalette(cp_StreamPalette, i_secondary_position + i * 64);
        //device_leds[i] = secondaryColor; // Overwrite with secondary color
      }
    }

    // Update positions for primary and secondary animations
    i_led_position += i_animation_step;
    //i_secondary_position += i_animation_step * 2; // Secondary moves faster
  }
}