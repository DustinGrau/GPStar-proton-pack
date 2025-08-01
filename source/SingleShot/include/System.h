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

// Forward function declarations
void checkEncoderAction();
void setPowerOnReminder(bool);
void ventTopLightControl(bool);
void ventLightControl(uint8_t);

void allLightsOff() {
  bargraph.off();

  // Turn off all non-addressable LEDs.
  led_Status.turnOff(); // Turn off the Neutrona Wand board status LED.
  led_Clippard.turnOff(); // Turn off the front left LED under the Clippard valve.
  led_Hat1.turnOff(); // Turn off hat light 1 (not used, but just make sure).
  led_Hat2.turnOff(); // Turn off hat light 2.
  led_SloBlo.turnOff();
  led_Tip.turnOff(); // Not used normally, but make sure it's off.
  ventLightControl(0);
  ventTopLightControl(false);

  // Clear all addressable LEDs by filling the array with black.
  fill_solid(system_leds, CYCLOTRON_LED_COUNT + BARREL_LED_COUNT, CRGB::Black);

  if(!b_playing_music) {
    // If music is not playing, arm the power-on reminder LED system.
    setPowerOnReminder(true);
  }
}

void allMenuLightsOff() {
  // Make sure some of the device lights are off, specifically for the Menu systems.
  // LEDs are in the order by which they indicate the menu levels above level 1.
  led_SloBlo.turnOff(); // Level 2
  ventLightControl(0); // Level 3
  ventTopLightControl(false); // Level 4
  led_Clippard.turnOff(); // Level 5

  setPowerOnReminder(false);
}

void barrelLightsOff() {
  ms_firing_pulse.stop();

  i_pulse_step = 0;

  // Turn off the barrel LED.
  for(uint8_t i = 0; i < i_num_barrel_leds; i++) {
    system_leds[i] = getHueAsRGB(C_BLACK);
  }

  // Turn off the device barrel tip LED.
  led_Tip.turnOff();
}

void vibrationOff() {
  ms_menu_vibration.stop();
  i_vibration_level_current = 0;
  analogWrite(VIBRATION_PIN, i_vibration_level_current);
}

void checkMenuVibration() {
  if(ms_menu_vibration.justFinished()) {
    vibrationOff();
  }
  else if(ms_menu_vibration.isRunning()) {
    if(i_vibration_level_current != i_vibration_level_min) {
      i_vibration_level_current = i_vibration_level_min;
      analogWrite(VIBRATION_PIN, i_vibration_level_current);
    }
  }
}

void switchLoops() {
  switch_intensify.poll();
  switch_activate.poll();
  switch_vent.poll();
  switch_device.poll();
  switch_grip.poll();
}

void soundIdleLoop(bool fadeIn) {
  switch(POWER_LEVEL) {
    case LEVEL_1:
    default:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 5000);
    break;
    case LEVEL_2:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 5000);
    break;
    case LEVEL_3:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 5000);
    break;
    case LEVEL_4:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 5000);
    break;
    case LEVEL_5:
      playEffect(S_IDLE_LOOP, true, i_volume_effects, fadeIn, 5000);
    break;
  }
}

void soundIdleLoopStop() {
  switch(POWER_LEVEL) {
    case LEVEL_1:
    default:
      stopEffect(S_IDLE_LOOP);
    break;
    case LEVEL_2:
      stopEffect(S_IDLE_LOOP);
    break;
    case LEVEL_3:
      stopEffect(S_IDLE_LOOP);
    break;
    case LEVEL_4:
      stopEffect(S_IDLE_LOOP);
    break;
    case LEVEL_5:
      stopEffect(S_IDLE_LOOP);
    break;
  }
}

// Checks the top rotary dial on the device.
void checkRotaryEncoder() {
  encoder.check(); // Update the latest state of the device resulting from any user input.
  checkEncoderAction(); // Take action specifically from interaction by the user.
}

void systemPOST() {
  uint8_t i_delay = 100;

  // Play a sound to test the audio system.
  playEffect(S_DEVICE_READY);

  // Turn on all bargraph elements and force an update
  bargraph.reset();
  bargraph.full();
  bargraph.commit();

  // These go HIGH to turn on.
  led_Status.turnOn();
  led_SloBlo.turnOn();
  delay(i_delay);
  led_Clippard.turnOn();
  delay(i_delay);
  led_Hat2.turnOn();
  delay(i_delay);

  if(b_rgb_vent_light) {
    // These are driven from the TopWhite LED pin.
    vent_leds[0] = getHueAsRGB(C_WARM_WHITE);
    FastLED[1].showLeds(255);
    delay(i_delay);
    vent_leds[1] = getHueAsRGB(C_WHITE);
    FastLED[1].showLeds(255);
    delay(i_delay);
  }
  #ifndef ESP32
  else {
    // These go LOW to turn on.
    led_Vent.turnOn();
    delay(i_delay);
    led_TopWhite.turnOn();
    delay(i_delay);
  }
  #endif

  // Optional barrel tip (could be alternate for the GPStar jewel)
  led_Tip.turnOn();
  delay(i_delay);

  // Sequentially turn on all LEDs in the barrel.
  for(uint8_t i = 0; i < i_num_barrel_leds; i++) {
    system_leds[i] = getHueAsRGB(C_BLUE);
    FastLED[0].showLeds(255);
    delay(i_delay);
  }

  // Sequentially turn on all LEDs in the cyclotron.
  for(uint8_t i = 0; i < i_num_cyclotron_leds; i++) {
    system_leds[i_cyclotron_led_start + i] = getHueAsRGB(C_RED);
    FastLED[0].showLeds(255);
    delay(i_delay);
  }

  // Turn on the front barrel.
  system_leds[i_barrel_led] = getHueAsRGB(C_WHITE);
  FastLED[0].showLeds(255);

  delay(i_delay * 8);

  allLightsOff(); // Turn off all lights, including the bargraph.

  // Make sure change to bargraph is immediate.
  bargraph.commit();
}

bool increasePowerLevel() {
  bool b_changed = true;

  switch(POWER_LEVEL) {
    case LEVEL_1:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_2;
    break;
    case LEVEL_2:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_3;
    break;
    case LEVEL_3:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_4;
    break;
    case LEVEL_4:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_5;
    break;
    case LEVEL_5:
      // No change, at highest level.
      b_changed = false;
    break;
  }

  if(b_changed) {
    playEffect(S_BEEPS);
  }

  // Returns true if value was changed.
  return b_changed;
}

bool decreasePowerLevel() {
  bool b_changed = true;

  switch(POWER_LEVEL) {
    case LEVEL_1:
      // No change, at lowest level.
      b_changed = false;
    break;
    case LEVEL_2:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_1;
    break;
    case LEVEL_3:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_2;
    break;
    case LEVEL_4:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_3;
    break;
    case LEVEL_5:
      POWER_LEVEL_PREV = POWER_LEVEL;
      POWER_LEVEL = LEVEL_4;
    break;
  }

  if(b_changed) {
    playEffect(S_BEEPS);
  }

  // Returns true if value was changed.
  return b_changed;
}

// Increasing the menu level means the user is going deeper
bool lowerMenuLevel() {
  bool b_changed = true;

  switch(DEVICE_MENU_LEVEL) {
    case MENU_LEVEL_1:
      if(DEVICE_STATUS == MODE_ON && DEVICE_ACTION_STATUS == ACTION_SETTINGS) {
        // Do not advance past level 1 for the settings menu when on.
        b_changed = false;
      }
      else {
        DEVICE_MENU_LEVEL = MENU_LEVEL_2;
        led_SloBlo.turnOn(); // Level 2
        ventLightControl(0); // Level 3
        ventTopLightControl(false); // Level 4
        led_Clippard.turnOff(); // Level 5

        stopEffect(S_BEEPS);
        playEffect(S_BEEPS);

        stopEffect(S_VOICE_LEVEL_1);
        playEffect(S_VOICE_LEVEL_2);
        stopEffect(S_VOICE_LEVEL_3);
        stopEffect(S_VOICE_LEVEL_4);
        stopEffect(S_VOICE_LEVEL_5);
      }
    break;
    case MENU_LEVEL_2:
      if(DEVICE_STATUS == MODE_OFF && DEVICE_ACTION_STATUS == ACTION_SETTINGS) {
        // Do not advance past level 2 for the settings menu when off.
        b_changed = false;
      }
      else {
        DEVICE_MENU_LEVEL = MENU_LEVEL_3;
        led_SloBlo.turnOn(); // Level 2
        ventLightControl(255); // Level 3
        ventTopLightControl(false); // Level 4
        led_Clippard.turnOff(); // Level 5

        stopEffect(S_BEEPS);
        playEffect(S_BEEPS);

        stopEffect(S_VOICE_LEVEL_1);
        stopEffect(S_VOICE_LEVEL_2);
        playEffect(S_VOICE_LEVEL_3);
        stopEffect(S_VOICE_LEVEL_4);
        stopEffect(S_VOICE_LEVEL_5);
      }
    break;
    case MENU_LEVEL_3:
      DEVICE_MENU_LEVEL = MENU_LEVEL_4;
      led_SloBlo.turnOn(); // Level 2
      ventLightControl(255); // Level 3
      ventTopLightControl(true); // Level 4
      led_Clippard.turnOff(); // Level 5

      stopEffect(S_BEEPS);
      playEffect(S_BEEPS);

      stopEffect(S_VOICE_LEVEL_1);
      stopEffect(S_VOICE_LEVEL_2);
      stopEffect(S_VOICE_LEVEL_3);
      playEffect(S_VOICE_LEVEL_4);
      stopEffect(S_VOICE_LEVEL_5);
    break;
    case MENU_LEVEL_4:
      DEVICE_MENU_LEVEL = MENU_LEVEL_5;
      led_SloBlo.turnOn(); // Level 2
      ventLightControl(255); // Level 3
      ventTopLightControl(true); // Level 4
      led_Clippard.turnOn(); // Level 5

      stopEffect(S_BEEPS);
      playEffect(S_BEEPS);

      stopEffect(S_VOICE_LEVEL_1);
      stopEffect(S_VOICE_LEVEL_2);
      stopEffect(S_VOICE_LEVEL_3);
      stopEffect(S_VOICE_LEVEL_4);
      playEffect(S_VOICE_LEVEL_5);
    break;
    case MENU_LEVEL_5:
      b_changed = false;
    break;
  }

  // Returns true if value was changed.
  return b_changed;
}

bool raiseMenuLevel() {
  bool b_changed = true;

  switch(DEVICE_MENU_LEVEL) {
    case MENU_LEVEL_1:
      // Menu level 1 is actually the top, so make sure all lights are off;
      b_changed = false;
    break;
    case MENU_LEVEL_2:
      DEVICE_MENU_LEVEL = MENU_LEVEL_1;
      led_SloBlo.turnOff(); // Level 2
      ventLightControl(0); // Level 3
      ventTopLightControl(false); // Level 4
      led_Clippard.turnOff(); // Level 5

      stopEffect(S_BEEPS);
      playEffect(S_BEEPS);

      playEffect(S_VOICE_LEVEL_1);
      stopEffect(S_VOICE_LEVEL_2);
      stopEffect(S_VOICE_LEVEL_3);
      stopEffect(S_VOICE_LEVEL_4);
      stopEffect(S_VOICE_LEVEL_5);
    break;
    case MENU_LEVEL_3:
      DEVICE_MENU_LEVEL = MENU_LEVEL_2;
      led_SloBlo.turnOn(); // Level 2
      ventLightControl(0); // Level 3
      ventTopLightControl(false); // Level 4
      led_Clippard.turnOff(); // Level 5

      stopEffect(S_BEEPS);
      playEffect(S_BEEPS);

      stopEffect(S_VOICE_LEVEL_1);
      playEffect(S_VOICE_LEVEL_2);
      stopEffect(S_VOICE_LEVEL_3);
      stopEffect(S_VOICE_LEVEL_4);
      stopEffect(S_VOICE_LEVEL_5);
    break;
    case MENU_LEVEL_4:
      DEVICE_MENU_LEVEL = MENU_LEVEL_3;
      led_SloBlo.turnOn(); // Level 2
      ventLightControl(255); // Level 3
      ventTopLightControl(false); // Level 4
      led_Clippard.turnOff(); // Level 5

      stopEffect(S_BEEPS);
      playEffect(S_BEEPS);

      stopEffect(S_VOICE_LEVEL_1);
      stopEffect(S_VOICE_LEVEL_2);
      playEffect(S_VOICE_LEVEL_3);
      stopEffect(S_VOICE_LEVEL_4);
      stopEffect(S_VOICE_LEVEL_5);
    break;
    case MENU_LEVEL_5:
      DEVICE_MENU_LEVEL = MENU_LEVEL_4;
      led_SloBlo.turnOn(); // Level 2
      ventLightControl(255); // Level 3
      ventTopLightControl(true); // Level 4
      led_Clippard.turnOff(); // Level 5

      stopEffect(S_BEEPS);
      playEffect(S_BEEPS);

      stopEffect(S_VOICE_LEVEL_1);
      stopEffect(S_VOICE_LEVEL_2);
      stopEffect(S_VOICE_LEVEL_3);
      playEffect(S_VOICE_LEVEL_4);
      stopEffect(S_VOICE_LEVEL_5);
    break;
  }

  // Returns true if value was changed.
  return b_changed;
}

bool decreaseOptionLevel() {
  bool b_changed = true;

  switch(MENU_OPTION_LEVEL) {
    case OPTION_1:
      MENU_OPTION_LEVEL = OPTION_2;
    break;
    case OPTION_2:
      MENU_OPTION_LEVEL = OPTION_3;
    break;
    case OPTION_3:
      MENU_OPTION_LEVEL = OPTION_4;
    break;
    case OPTION_4:
      MENU_OPTION_LEVEL = OPTION_5;
    break;
    case OPTION_5:
      if(raiseMenuLevel()) {
        MENU_OPTION_LEVEL = OPTION_1;
      }
      else {
        b_changed = false;
      }
    break;
  }

  // Returns true if value was changed.
  return b_changed;
}

bool increaseOptionLevel() {
  bool b_changed = true;

  switch(MENU_OPTION_LEVEL) {
    case OPTION_1:
      if(lowerMenuLevel()) {
        MENU_OPTION_LEVEL = OPTION_5;
      }
      else {
        b_changed = false;
      }
    break;
    case OPTION_2:
      MENU_OPTION_LEVEL = OPTION_1;
    break;
    case OPTION_3:
      MENU_OPTION_LEVEL = OPTION_2;
    break;
    case OPTION_4:
      MENU_OPTION_LEVEL = OPTION_3;
    break;
    case OPTION_5:
      MENU_OPTION_LEVEL = OPTION_4;
    break;
  }

  // Returns true if value was changed.
  return b_changed;
}

void checkCyclotron() {
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
}

void firePulseEffect() {
  if(i_pulse_step == 0) {
    // Force clear and reset of bargraph state.
    bargraph.clear();
    bargraph.reset();
    bargraph.commit();

    // Change bargraph animation when pulse begins.
    bargraph.PATTERN = BG_OUTER_INNER;
  }

  // Strobe the barrel LED while firing in case someone isn't using an addressable LED.
  i_pulse_step % 2 == 0 ? led_Tip.turnOn() : led_Tip.turnOff();

  // Primary blast.
  switch(i_pulse_step) {
    case 0:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED);
    break;
    case 1:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED3);
    break;
    case 2:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED5);
    break;
    case 3:
      system_leds[i_barrel_led] = getHueAsRGB(C_WHITE);
    break;
    case 4:
      system_leds[i_barrel_led] = getHueAsRGB(C_BLACK);

      for(uint8_t i = 0; i < i_num_barrel_leds - 1; i++) {
        system_leds[i] = getHueAsRGB(C_WHITE);
      }
    break;
    case 5:
      system_leds[i_barrel_led] = getHueAsRGB(C_WHITE);

      for(uint8_t i = 0; i < i_num_barrel_leds - 1; i++) {
        system_leds[i] = getHueAsRGB(C_BLACK);
      }
    break;
    case 6:
      system_leds[i_barrel_led] = getHueAsRGB(C_BLACK);

      for(uint8_t i = 0; i < i_num_barrel_leds - 1; i++) {
        system_leds[i] = getHueAsRGB(C_WHITE);
      }
    break;
    case 7:
      system_leds[i_barrel_led] = getHueAsRGB(C_WHITE);

      for(uint8_t i = 0; i < i_num_barrel_leds - 1; i++) {
        system_leds[i] = getHueAsRGB(C_BLACK);
      }
    break;
    case 8:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED4);
    break;
    case 9:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED2);
    break;
    case 10:
      system_leds[i_barrel_led] = getHueAsRGB(C_RED);
    break;
    case 11:
      system_leds[i_barrel_led] = getHueAsRGB(C_BLACK);
    break;
    default:
      // This is an invalid state, so turn off all the LEDs.
      system_leds[i_barrel_led] = getHueAsRGB(C_BLACK);
      led_Tip.turnOff();
    break;
  }

  i_pulse_step++;

  if(i_pulse_step < i_pulse_step_max) {
    ms_firing_pulse.start(i_firing_pulse);
  }
  else {
    // Animation has concluded, so reset our timer and variable.
    ms_firing_pulse.stop();
    i_pulse_step = 0;

    // Clear the bargraph and return to ramping.
    bargraph.clear();
    bargraph.PATTERN = BG_POWER_RAMP;
  }
}

void checkGeneralTimers() {
  // Play the firing pulse effect animation if timer completed.
  if(ms_firing_pulse.justFinished()) {
    firePulseEffect(); // Single shot animation.
  }

  // Update the timer for the slo-blo blink.
  if(ms_slo_blo_blink.justFinished()) {
    ms_slo_blo_blink.start(i_slo_blo_blink_delay);
  }
}

void modeFireStart() {
  //modeFireStartSounds();

  // Just in case a semi-auto was fired before we started firing a stream, stop its timer.
  ms_semi_automatic_firing.stop();

  // Turn on hat light 1.
  led_Hat1.turnOn();

  barrelLightsOff();
}

void modeFireStopSounds() {
  // Reset some sound triggers.
  b_sound_firing_intensify_trigger = false;
  b_sound_firing_alt_trigger = false;

  ms_single_blast.stop();
}

void modeFireStop() {
  DEVICE_ACTION_STATUS = ACTION_IDLE;

  b_firing = false;
  b_firing_intensify = false;
  b_firing_alt = false;

  led_Hat2.turnOn(); // Make sure we turn on hat light 2 in case it's off as well.

  led_Tip.turnOff();

  ms_warning_blink.stop();

  modeFireStopSounds();
}

void modeFiring() {
  // Sound trigger flags.
  if(b_firing_intensify && !b_sound_firing_intensify_trigger) {
    b_sound_firing_intensify_trigger = true;
  }

  if(!b_firing_intensify && b_sound_firing_intensify_trigger) {
    b_sound_firing_intensify_trigger = false;
  }

  if(b_firing_alt && !b_sound_firing_alt_trigger) {
    b_sound_firing_alt_trigger = true;
  }

  if(!b_firing_alt && b_sound_firing_alt_trigger) {
    b_sound_firing_alt_trigger = false;
  }
}

void ventTopLightControl(bool b_on) {
  if(!b_on) {
    #ifndef ESP32
    if(!b_rgb_vent_light) {
      // Turn off top light.
      led_TopWhite.turnOff();
    }
    #endif

    // Turn off if not off already.
    if(vent_leds[1]) {
      vent_leds[1] = getHueAsRGB(C_BLACK);
      b_vent_lights_changed = true;
    }
  }
  else {
    #ifndef ESP32
    if(!b_rgb_vent_light) {
      // Turn on top light.
      led_TopWhite.turnOn();
    }
    #endif

    // Turn on if not on already.
    if(!vent_leds[1]) {
      vent_leds[1] = getHueAsRGB(C_WHITE);
      b_vent_lights_changed = true;
    }
  }
}

void ventLightControl(uint8_t i_intensity) {
  if(b_rgb_vent_light) {
    // Put in a check just to be sure the non-addressable pin stays off.
  #ifndef ESP32
    if(led_Vent.getState() != HIGH) {
      led_Vent.turnOff();
    }
  #endif

    if(i_intensity < 20) {
      vent_leds[0] = getHueAsRGB(C_BLACK);
    }
    else {
      vent_leds[0] = getHueAsRGB(C_WARM_WHITE, i_intensity);
    }

    b_vent_lights_changed = true;
  }
  #ifndef ESP32
  else {
    if(i_intensity < 1) {
      led_Vent.turnOff();
    }
    else {
      led_Vent.dim(255 - PROGMEM_READU8(ledLookupTable[i_intensity]));
    }
  }
  #endif
}

// Determine the light status on the device and any beeps.
void deviceLightControlCheck() {
  // Vent light and first stage of the safety system.
  if(switch_vent.on()) {
    if(b_vent_light_control) {
      // Vent light on, brightness dependent on mode.
      if(DEVICE_ACTION_STATUS == ACTION_FIRING || (ms_semi_automatic_firing.isRunning() && !ms_semi_automatic_firing.justFinished())) {
        ventLightControl(255);
      }
      else {
        // Adjust brightness based on the power level.
        switch(POWER_LEVEL) {
          case LEVEL_1:
          default:
            ventLightControl(i_vent_led_power_1);
          break;
          case LEVEL_2:
            ventLightControl(i_vent_led_power_2);
          break;
          case LEVEL_3:
            ventLightControl(i_vent_led_power_3);
          break;
          case LEVEL_4:
            ventLightControl(i_vent_led_power_4);
          break;
          case LEVEL_5:
            ventLightControl(i_vent_led_power_5);
          break;
        }
      }
    }
    else {
      ventLightControl(255);
    }
  }
  else if(!switch_vent.on()) {
    // Vent light off.
    ventLightControl(0);
  }
}

void deviceOff() {
  if(b_device_boot_error_on) {
    stopEffect(S_BEEPS_LOW);
    stopEffect(S_BEEPS);
    stopEffect(S_BEEPS);
  }

  stopEffect(S_BOOTUP);

  if(DEVICE_ACTION_STATUS == ACTION_ERROR && !b_device_boot_error_on) {
    // We are exiting Device Boot Error, so change device state back to off/idle.
    DEVICE_STATUS = MODE_OFF;
    DEVICE_ACTION_STATUS = ACTION_IDLE;
  }
  else if(DEVICE_ACTION_STATUS != ACTION_ERROR && b_device_boot_error_on) {
    // We are entering either Device Boot Error mode or Button Mash Timeout mode, so do nothing.
  }
  else {
    // Full device shutdown in all other situations.
    DEVICE_STATUS = MODE_OFF;
    DEVICE_ACTION_STATUS = ACTION_IDLE;

    stopEffect(S_SHUTDOWN);
    playEffect(S_SHUTDOWN);
  }

  soundIdleLoopStop();

  vibrationOff();

  // Stop firing if the device is turned off.
  if(b_firing) {
    modeFireStop();
  }

  // Turn off some timers.
  ms_cyclotron.stop();
  ms_settings_blink.stop();
  ms_semi_automatic_check.stop();
  ms_semi_automatic_firing.stop();
  ms_warning_blink.stop();
  ms_error_blink.stop();

  switch(DEVICE_STATUS) {
    case MODE_OFF:
      // Turn off all device lights.
      allLightsOff();

      deviceSwitchedCount = 0;
      ventSwitchedCount = 0;
    break;
    default:
      // Do nothing if we aren't MODE_OFF
    break;
  }
}

void modeError() {
  deviceOff();

  DEVICE_STATUS = MODE_ERROR;
  DEVICE_ACTION_STATUS = ACTION_ERROR;

  // This is used for controlling the bargraph beeping while in boot error mode.
  ms_warning_blink.start(i_bargraph_beep_delay);
  ms_error_blink.start(i_error_blink_delay);
  ms_settings_blink.start(i_settings_blink_delay);

  playEffect(S_BEEPS_LOW);
  playEffect(S_BEEPS);
  playEffect(S_BEEPS);
}

void modePulseStart() {
  // Handles all "pulsed" fire modes.
  barrelLightsOff();

  playEffect(S_FIRE_BLAST, false, i_volume_effects, false, 0, false);

  ms_firing_pulse.start(i_firing_pulse);
  ms_semi_automatic_firing.start(350);
}

// Check if we should fire, or if the device was turned off.
void fireControlCheck() {
  // Firing action stuff and shutting cyclotron and the Single-Shot Blaster off.
  if(DEVICE_ACTION_STATUS != ACTION_SETTINGS) {
    // If Activate switch is down, turn device off.
    if(!switch_activate.on()) {
      DEVICE_ACTION_STATUS = ACTION_OFF;
      return;
    }

    // Otherwise the Activate switch is up, so check if in a firing state.
    if(switch_device.on() && switch_vent.on()) {
      if(switch_grip.on()) {
        b_firing_alt = true;

        switch(STREAM_MODE) {
          case PROTON:
            // Handle Primary Blast fire start here.
            if(!b_firing_semi_automatic && ms_semi_automatic_check.remaining() < 1) {
              // Start rate-of-fire timer.
              ms_semi_automatic_check.start(i_single_shot_rate);

              modePulseStart();

              b_firing_semi_automatic = true;
            }
          break;

          default:
            // Do nothing.
          break;
        }
      }

      if(!switch_intensify.on()) {
        switch(STREAM_MODE) {
          case PROTON:
          default:
            if(b_firing && b_firing_intensify) {
              if(!b_firing_alt) {
                DEVICE_ACTION_STATUS = ACTION_IDLE;
              }

              b_firing_intensify = false;
            }
          break;
        }
      }

      if(!switch_grip.on()) {
        switch(STREAM_MODE) {
          case PROTON:
            // Handle resetting semi-auto bool here.
            b_firing_semi_automatic = false;
          break;

          default:
            // Do nothing.
          break;
        }
      }
    }
  }
  else if(DEVICE_ACTION_STATUS == ACTION_SETTINGS) {
    // If Activate switch is down, turn device off.
    if(!switch_activate.on()) {
      DEVICE_ACTION_STATUS = ACTION_OFF;
      return;
    }
  }
}

void postActivation() {
  if(DEVICE_STATUS != MODE_ERROR) {
    // Turn on slo-blo light.
    led_SloBlo.turnOn();

    // Turn on the Clippard LED.
    led_Clippard.turnOn();

    // Top white light.
    ms_white_light.start(i_top_blink_interval);
    ventTopLightControl(true);

    // Reset the hat light timers.
    ms_warning_blink.stop();
    ms_error_blink.stop();

    stopEffect(S_BOOTUP);
    playEffect(S_BOOTUP);

    soundIdleLoop(true);

    if(bargraph.STATE == BG_OFF) {
      bargraph.reset(); // Enable bargraph for use (resets variables and turns it on).
      bargraph.PATTERN = BG_POWER_RAMP; // Bargraph idling loop.
    }
  }
}

void modeActivate() {
  setPowerOnReminder(false);

  // The device was started while the top switch was already on, so let's put the device into startup error mode.
  if(switch_device.on() && b_device_boot_errors) {
    b_device_boot_error_on = true;
    modeError();
  }
  else {
    // Device is officially activated and on.
    DEVICE_STATUS = MODE_ON;

    // Proper startup. Continue booting up the device.
    DEVICE_ACTION_STATUS = ACTION_IDLE;
  }

  postActivation(); // Enable lights and bargraph after device activation.
}

void vibrationDevice(uint8_t i_level) {
  if(VIBRATION_MODE != VIBRATION_NONE && i_level > 0) {
    // Vibrate the device during firing only when enabled.
    if(VIBRATION_MODE == VIBRATION_FIRING_ONLY) {
      if(ms_semi_automatic_firing.isRunning()) {
        if(i_vibration_level_current != (i_level * 2 < 64 ? i_level * 2 : 64)) {
          i_vibration_level_current = (i_level * 2 < 64 ? i_level * 2 : 64);
          analogWrite(VIBRATION_PIN, i_vibration_level_current);
        }
      }
      else {
        vibrationOff();
      }
    }
    else {
      // Device vibrates even when idling, etc.
      if(ms_semi_automatic_firing.isRunning()) {
        if(i_vibration_level_current != (i_level * 2 < 64 ? i_level * 2 : 64)) {
          i_vibration_level_current = (i_level * 2 < 64 ? i_level * 2 : 64);
          analogWrite(VIBRATION_PIN, i_vibration_level_current);
        }
      }
      else if(i_vibration_level_current != i_level) {
        i_vibration_level_current = i_level;
        analogWrite(VIBRATION_PIN, i_level);
      }
    }
  }
  else {
    vibrationOff();
  }
}

void vibrationSetting() {
  switch(POWER_LEVEL) {
    case LEVEL_1:
    default:
      vibrationDevice(i_vibration_level_min);
    break;

    case LEVEL_2:
      vibrationDevice(i_vibration_level_min + 5);
    break;

    case LEVEL_3:
      vibrationDevice(i_vibration_level_min + 10);
    break;

    case LEVEL_4:
      vibrationDevice(i_vibration_level_min + 12);
    break;

    case LEVEL_5:
      vibrationDevice(i_vibration_level_min + 25);
    break;
  }
}

void ventSwitched(void* n) {
  (void)(n); // Suppress unused variable warning
  ventSwitchedCount++;
}

void deviceSwitched(void* n) {
  (void)(n); // Suppress unused variable warning
  deviceSwitchedCount++;
}

// Enter the device menu system.
void deviceEnterMenu() {
  debug(F("deviceEnterMenu|"));
  debugln(DEVICE_ACTION_STATUS);

  // Enter a menu at level 1, at option #5
  DEVICE_MENU_LEVEL = MENU_LEVEL_1;
  MENU_OPTION_LEVEL = OPTION_5;

  playEffect(S_CLICK);

  allLightsOff();
  allMenuLightsOff();

  bargraph.showBars(MENU_OPTION_LEVEL);
}

// Exit the device menu system while the device is off.
void deviceExitMenu() {
  debug(F("deviceExitMenu|"));
  debugln(DEVICE_ACTION_STATUS);

  // Reset the menu level/option to default
  DEVICE_MENU_LEVEL = MENU_LEVEL_1;
  MENU_OPTION_LEVEL = OPTION_5;

  playEffect(S_CLICK);

  DEVICE_ACTION_STATUS = ACTION_IDLE;

  allLightsOff();

  if(DEVICE_STATUS == MODE_ON && bargraph.STATE == BG_OFF) {
    bargraph.reset(); // Enable bargraph for use (resets variables and turns it on).
    bargraph.PATTERN = BG_POWER_RAMP; // Bargraph idling loop.
    led_SloBlo.turnOn(); // Turn on SLO-BLO if device is on.
  }
}

// Exit the device menu EEPROM system while the device is off.
void deviceExitEEPROMMenu() {
  debug(F("deviceExitEEPROMMenu|"));
  debugln(DEVICE_ACTION_STATUS);

  // Reset the menu level/option to default
  DEVICE_MENU_LEVEL = MENU_LEVEL_1;
  MENU_OPTION_LEVEL = OPTION_5;

  playEffect(S_BEEPS);

  DEVICE_ACTION_STATUS = ACTION_IDLE;

  allLightsOff();

  deviceSwitchedCount = 0;
  ventSwitchedCount = 0;

  vibrationOff(); // Make sure we stop any menu-related vibration, if any.
}