/**
 *   GPStar BeltGizmo - Ghostbusters Props, Mods, and Kits.
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
  // Turn off the RGB pins.
  digitalWrite(LED_R_PIN, LOW);
  digitalWrite(LED_G_PIN, LOW);
  digitalWrite(LED_B_PIN, LOW);
  fill_solid(device_leds, DEVICE_NUM_LEDS, CRGB::Black);
}

// Animates the LEDs in a wave-like pattern
void animateLights() {
  static uint16_t i_led_position = 0;

  // Update timer interval in case i_power changes
  if (ms_anim_change.justFinished()) {
    if(b_firing) {
      // Speed up animation only when firing.
      ms_anim_change.start(i_animation_duration / ((i_power + 1) * 2));
    }
    else {
      // Otherwise return to normal speed.
      ms_anim_change.start(i_animation_duration);
    }

    for (int i = 0; i < DEVICE_NUM_LEDS; i++) {
      uint8_t i_brightness = map(sin8((i_led_position + i * 32) % 255), 0, 255, i_min_brightness, i_max_brightness);
      //device_leds[i] = CHSV((i_led_position + i * 20) % 255, 255, 255);
      
      switch(STREAM_MODE) {
        case PROTON:
          // Red
          device_leds[i] = getHueAsRGB(PRIMARY_LED, C_RED, 255 - i_brightness);
        break;
        case SLIME:
          // Green
          device_leds[i] = getHueAsRGB(PRIMARY_LED, C_GREEN, 255 - i_brightness);
        break;
        case STASIS:
          // Blue
          device_leds[i] = getHueAsRGB(PRIMARY_LED, C_BLUE, 255 - i_brightness);
        break;
        case MESON:
          // Orange
          device_leds[i] = getHueAsRGB(PRIMARY_LED, C_ORANGE, 255 - i_brightness);
        break;
        case SPECTRAL:
          // Rainbow
          device_leds[i] = getHueAsRGB(PRIMARY_LED, C_RAINBOW, 255 - i_brightness);
        break;
        default:
          // White (Holiday/Custom)
          device_leds[i] = getHueAsRGB(PRIMARY_LED, C_WHITE, 255 - i_brightness);
        break;
      }
    }

    i_led_position += i_animation_step; // Move the wave position by shifting position for the next update.
  }
}

void blinkLights() {
  static bool b_restart = true;
  static uint8_t i_current = 0;
  static uint16_t i_delay_total;
  static uint16_t i_delay_led;

  if(b_firing) {
    // Only perform blinking if firing.
    if (i_current == 0) {
      b_restart = true; // Blink when sequence is complete.
    }
    else {
      b_restart = false; // Otherwise the animation is ongoing.
    }

    // Increment the count for the animation sequence.
    i_current++;
    if (i_current > 1) {
      // Ensure we cycle back around in the sequence.
      i_current = i_current % DEVICE_NUM_LEDS;
    }

    if(ms_anim_change.remaining() < 1) {
      if(i_power > 0) {
        // Speed up the blink with the power level.
        i_delay_total = i_animation_time / i_power;
        
      }
      else {
        // Handle case where power is unset.
        i_delay_total = i_animation_time;
      }
      ms_anim_change.start(i_delay_total);
      i_delay_led = i_delay_total / DEVICE_NUM_LEDS;
    }

    if(b_restart) {
      ledsOff(); // Turn off LEDs when flag is true.
    }
    else {
      // Turn on LED's according to the firing mode.
      switch(STREAM_MODE) {
        case PROTON:
          // Red
          digitalWrite(LED_R_PIN, HIGH);
          device_leds[i_current] = getHueAsRGB(PRIMARY_LED, C_RED);
        break;
        case SLIME:
          // Green
          digitalWrite(LED_G_PIN, HIGH);
          device_leds[i_current] = getHueAsRGB(PRIMARY_LED, C_GREEN);
        break;
        case STASIS:
          // Blue
          digitalWrite(LED_B_PIN, HIGH);
          device_leds[i_current] = getHueAsRGB(PRIMARY_LED, C_BLUE);
        break;
        case MESON:
          // Orange
          digitalWrite(LED_R_PIN, HIGH);
          digitalWrite(LED_G_PIN, HIGH);
          device_leds[i_current] = getHueAsRGB(PRIMARY_LED, C_ORANGE);
        break;
        default:
          // White
          digitalWrite(LED_R_PIN, HIGH);
          digitalWrite(LED_G_PIN, HIGH);
          digitalWrite(LED_B_PIN, HIGH);
          device_leds[i_current] = getHueAsRGB(PRIMARY_LED, C_RAINBOW);
        break;
      }
    }
  }
  else {
    ledsOff(); // Turn off the RGB LED's
    b_restart = true; // Mark animation as reset.
    i_current = 0; // Reset the LED sequence.
  }
}
