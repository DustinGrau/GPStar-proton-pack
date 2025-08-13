/**
 *   GPStar Proton Pack - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023-2025 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
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

// Required for PlatformIO
#include <Arduino.h>

// Set to 1 to enable built-in debug messages via Serial device output.
#define DEBUG 1

// Debug macros
#if DEBUG == 1
  #define debug(...) Serial.print(__VA_ARGS__)
  #define debugf(...) Serial.printf(__VA_ARGS__)
  #define debugln(...) Serial.println(__VA_ARGS__)
#else
  #define debug(...)
  #define debugf(...)
  #define debugln(...)
#endif

// PROGMEM macros
#define PROGMEM_READU32(x) pgm_read_dword_near(&(x))
#define PROGMEM_READU16(x) pgm_read_word_near(&(x))
#define PROGMEM_READU8(x) pgm_read_byte_near(&(x))

// 3rd-Party Libraries
#include <CRC32.h>
#include <digitalWriteFast.h>
#include <EEPROM.h>
#include <millisDelay.h>
#include <FastLED.h>
#include <ezButton.h>
#include <Ramp.h>
#include <SerialTransfer.h>
#include <Wire.h>
#ifdef ESP32
  #include <HDC1080.h>
  GuL::HDC1080 tempSensor(Wire1);
  millisDelay ms_temp_read;
  bool b_temp_sensor_detected = false;
  #include <HardwareSerial.h>
#endif

// Forward declaration for use in all includes.
void sendDebug(String message);

// Local Files
#include "Configuration.h"
#include "MusicSounds.h"
#include "Communication.h"
#include "Header.h"
#include "Colours.h"
#include "Audio.h"
#include "PowerMeter.h"
#ifdef ESP32
  #include "PreferencesESP.h"
#else
  #include "PreferencesATMega.h"
#endif
#include "System.h"
#include "Command.h"
#include "Serial.h"
#ifdef ESP32
  #include "Wireless.h"
#endif

// Writes a debug message to the serial console or sends to the WebSocket.
void sendDebug(String message) {
  #if defined(DEBUG_SEND_TO_CONSOLE)
    debugln(message); // Print to serial console.
  #endif
  #if defined(DEBUG_SEND_TO_WEBSOCKET) and defined(ESP32)
    if (b_ws_started) {
      ws.textAll(message); // Send a copy to the WebSocket.
    }
  #endif
}

void setup() {
#ifdef ESP32
  // To save power, reduce CPU frequency to 160 MHz.
  setCpuFrequencyMhz(160);

  // Serial0 (UART0) is enabled by default; end() sets GPIO43 & GPIO44 to GPIO.
  Serial0.end();

  // Set the baud rate for the Serial console.
  Serial.begin(115200);

  /* This loop changes GPIO39~GPIO42 to Function 1, which is GPIO.
   * PIN_FUNC_SELECT sets the IOMUX function register appropriately.
   * IO_MUX_GPIO0_REG is the register for GPIO0, which we then seek from.
   * PIN_FUNC_GPIO is a define for Function 1, which sets the pins to GPIO mode.
   */
  for(uint8_t gpio_pin = 39; gpio_pin < 43; gpio_pin++) {
    PIN_FUNC_SELECT(IO_MUX_GPIO0_REG + (gpio_pin * 4), PIN_FUNC_GPIO);
  }

  // Assign AttenuatorSerial to pins 11/10 for the Attenuator/Wireless communications.
  AttenuatorSerial.begin(9600, SERIAL_8N1, ATTENUATOR_RX_PIN, ATTENUATOR_TX_PIN);

  // Assign Serial2 to pins 44/43 for the Neutrona Wand communications.
  WandSerial.begin(9600, SERIAL_8N1, WAND_RX_PIN, WAND_TX_PIN);
#else
  Serial.begin(9600); // Standard HW serial (USB) console.
  AttenuatorSerial.begin(9600); // Add-on Attenuator communication (19/18).
  WandSerial.begin(9600); // Communication to the Neutrona Wand (17/16).
#endif

  // Initialize the SerialTransfer objects by passing in the appropriate ports.
  attenuatorComs.begin(AttenuatorSerial, false, Serial, 100); // Attenuator/Wireless
  wandComs.begin(WandSerial, false); // Neutrona Wand

  // Setup the audio device for this controller.
  setupAudioDevice();

  // Setup the i2c bus using the Wire protocol.
#ifdef ESP32
  // ESP32-S3 requires manually specifying SDA and SCL pins first.
  Wire.begin(I2C_SDA, I2C_SCL, 400000UL);
  Wire1.begin(TEMP_SDA, TEMP_SCL, 400000UL);

  // Initialize the HDC1080 temp/humidity sensor.
  Wire1.beginTransmission(0x40);
  if(Wire1.endTransmission() == 0) {
    b_temp_sensor_detected = true;
    tempSensor.resetConfiguration();
    tempSensor.disableHeater();
    tempSensor.setHumidityResolution(GuL::HDC1080::HumidityMeasurementResolution::HUM_RES_14BIT);
    tempSensor.setTemperaturResolution(GuL::HDC1080::TemperatureMeasurementResolution::TEMP_RES_14BIT);
    tempSensor.setAcquisitionMode(GuL::HDC1080::AcquisitionModes::SINGLE_CHANNEL);
  }
#else
  Wire.begin();
  Wire.setClock(400000UL); // Sets the i2c bus to 400kHz
#endif

  // Initialize an optional power meter on the i2c bus.
  if(b_use_power_meter) {
    sendDebug(F("Init power meter..."));
    powerMeterInit();
  }

  // Rotary encoder for volume control.
  pinModeFast(ROTARY_ENCODER_A, INPUT_PULLUP);
  pinModeFast(ROTARY_ENCODER_B, INPUT_PULLUP);

  // Status indicator LED on the v1.5 GPStar Proton Pack Board.
  pinModeFast(PACK_STATUS_LED_PIN, OUTPUT);

  // Configure the various switches on the pack.
  switch_power.setDebounceTime(50);
  switch_alarm.setDebounceTime(50);
  switch_mode.setDebounceTime(50);
  switch_vibration.setDebounceTime(50);
  switch_cyclotron_lid.setDebounceTime(50);
#ifndef ESP32
  switch_cyclotron_direction.setDebounceTime(50);
  switch_smoke.setDebounceTime(50);
#endif

// Change PWM frequency of pin 45 for the vibration motor, we do not want it high pitched.
#ifdef ESP32
  // Use of the register is not needed by ESP32, as it uses a different method for PWM.
#else
  // For ATmega2560, we set the PWM frequency for pin 45 (TCCR5B) to 122.55 Hz.
  TCCR5B = (TCCR5B & B11111000) | B00000100;
  pinMode(VIBRATION_PIN, OUTPUT); // Vibration motor is PWM, so fallback to default pinMode just to be safe.
#endif

#ifdef ESP32
  // Begin by setting up WiFi as a prerequisite to all else.
  if(startWiFi()) {
    // Start the local web server.
    startWebServer();

    // Begin timer for remote client events.
    ms_cleanup.start(i_websocketCleanup);
    ms_apclient.start(i_apClientCount);
    ms_otacheck.start(i_otaCheck);
  }
#endif

  // Smoke motor for the N-Filter.
  pinModeFast(NFILTER_SMOKE_PIN, OUTPUT);

  // Fan pin for the N-Filter smoke.
  pinModeFast(NFILTER_FAN_PIN, OUTPUT);

  // Second smoke motor (booster tube)
  pinModeFast(BOOSTER_TUBE_SMOKE_PIN, OUTPUT);

  // A fan pin that goes off at the same time as the booster tube smoke pin.
  pinModeFast(BOOSTER_TUBE_FAN_PIN, OUTPUT);

  // Another optional N-Filter LED.
  pinModeFast(NFILTER_LED_PIN, OUTPUT);

  // Power Cell, Cyclotron Lid, and N-Filter.
  FastLED.addLeds<NEOPIXEL, PACK_LED_PIN>(pack_leds, FRUTTO_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT).setCorrection(TypicalLEDStrip);
  FastLED.setMaxRefreshRate(0); // Disable FastLED's blocking 2.5ms delay.

  // Inner Cyclotron LEDs (Inner Panel + Cyclotron + Cavity).
  FastLED.addLeds<NEOPIXEL, CYCLOTRON_LED_PIN>(cyclotron_leds, INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX).setCorrection(TypicalLEDStrip);

#ifndef ESP32
  // Cyclotron Switch Panel LEDs
  pinModeFast(CYCLOTRON_SWITCH_LED_R1_PIN, OUTPUT);
  pinModeFast(CYCLOTRON_SWITCH_LED_R2_PIN, OUTPUT);
  pinModeFast(CYCLOTRON_SWITCH_LED_Y1_PIN, OUTPUT);
  pinModeFast(CYCLOTRON_SWITCH_LED_Y2_PIN, OUTPUT);
  pinModeFast(CYCLOTRON_SWITCH_LED_G1_PIN, OUTPUT);
  pinModeFast(CYCLOTRON_SWITCH_LED_G2_PIN, OUTPUT);
  pinModeFast(YEAR_TOGGLE_LED_PIN, OUTPUT);
  pinModeFast(VIBRATION_TOGGLE_LED_PIN, OUTPUT);
#endif

  // Default mode is Super Hero (for simpler controls).
  SYSTEM_MODE = MODE_SUPER_HERO;

  // Bootup the pack into Proton mode, the same as the wand.
  STREAM_MODE = PROTON;

  // Set the CTS to not firing.
  STATUS_CTS = CTS_NOT_FIRING;

  // Set default year selection to toggle switch.
  SYSTEM_EEPROM_YEAR = SYSTEM_TOGGLE_SWITCH;

  // Set default vibration mode.
  VIBRATION_MODE_EEPROM = VIBRATION_DEFAULT;
  VIBRATION_MODE = VIBRATION_FIRING_ONLY;

  // Configure the vibration state.
  if(switch_vibration.getState() == LOW) {
    b_vibration_switch_on = true;
  }
  else {
    b_vibration_switch_on = false;
  }

  // Configure the year mode, though this will be modified
  // as based on the user's stored preferences in EEPROM.
  if(switch_mode.getState() == LOW) {
    SYSTEM_YEAR = SYSTEM_1984;
  }
  else {
    SYSTEM_YEAR = SYSTEM_AFTERLIFE;
  }
  SYSTEM_YEAR_TEMP = SYSTEM_YEAR;

  // Set a default for the cyclotron inner panel.
  INNER_CYC_PANEL_MODE = PANEL_RGB_DYNAMIC;

  // Load any saved settings stored in the EEPROM memory of the Proton Pack.
  if(b_eeprom) {
    readEEPROM();
  }

  // Reset the master volume. Important to keep this as we startup the system at the lowest volume.
  // Then the EEPROM reads any settings if required, then we reset the volume.
  updateMasterVolume(true);

  // Setup and configure the Inner Cyclotron LEDs.
  resetInnerCyclotronLEDs();
  updateProtonPackLEDCounts();

  // Check some LED brightness settings for various LEDs.
  // The datatype used should avoid checks for negative values.
  if(i_powercell_brightness > 100) {
    i_powercell_brightness = 100;
  }

  if(i_cyclotron_brightness > 100) {
    i_cyclotron_brightness = 100;
  }

  if(i_cyclotron_inner_brightness > 100) {
    i_cyclotron_inner_brightness = 100;
  }

  // Reset cyclotron ramps.
  resetRampSpeeds();

  // Start some timers
  ms_fast_led.start(i_fast_led_delay);
  ms_check_music.start(i_music_check_delay);
  ms_attenuator_check.start(i_attenuator_disconnect_delay);
  ms_cyclotron_switch_plate_leds.start(i_cyclotron_switch_plate_leds_delay);

  // Perform initial pack reset.
  packOffReset();

  if(SYSTEM_MODE == MODE_SUPER_HERO) {
    // Auto start the pack if it is in demo light mode.
    if(b_demo_light_mode) {
      // Turn the pack on.
      PACK_ACTION_STATE = ACTION_ACTIVATE;
    }
  }

  // Perform power-on sequence if demo light mode is not enabled per user preferences.
  if(!b_demo_light_mode) {
    // System Power On Self Test
    playEffect(S_POWER_ON);
    ms_delay_post.start(0);
  }
  else {
    b_pack_post_finish = true;
  }

#ifdef ESP32
  debugf("Setup complete, free heap: %u bytes\n", ESP.getFreeHeap());
#endif
}

void mainLoop() {
  if(b_pack_post_finish) {
    checkMusic();
    checkSwitches();
    checkRotaryEncoder();
    checkMenuVibration();

    // Check current voltage/amperage draw using available methods if enabled.
    if(b_use_power_meter) {
      // Only check if power meter if present and self-test has completed.
      checkPowerMeter();
    }

    switch(PACK_STATE) {
      case MODE_OFF:
        // Turn on the status indicator LED.
        digitalWriteFast(PACK_STATUS_LED_PIN, HIGH);

        if(PACK_ACTION_STATE == ACTION_IDLE && ms_delay_post.justFinished()) {
          // Brass Pack shutdown steam effect.
          playEffect(PROGMEM_READU16(sfx_smoke[random(5)]));
        }

        if(b_pack_on) {
          b_ramp_up = false;
          b_ramp_up_start = false;
          b_inner_ramp_up = false;
          b_fade_out = true;

          resetRampDown();

          b_pack_shutting_down = true;

          ms_fadeout.start(0);

          // Tell the wand the pack is off, so shut down the wand if it happens to still be on.
          packSerialSend(P_OFF);
          attenuatorSend(A_PACK_OFF);

          b_pack_on = false;
        }

        if(b_ramp_down && !b_overheating && !b_pack_alarm) {
          if(b_spectral_lights_on) {
            // If we enter the LED EEPROM menu while the pack is ramping off, stop it right away.
            packOffReset();
            spectralLightsOn();
          }
          else {
            cyclotronControl();
            cyclotronSwitchLEDLoop();
            powercellLoop();
          }
        }
        else {
          if(!b_spectral_lights_on) {
            if(ms_fadeout.justFinished()) {
              if(fadeOutCyclotron()) {
                ms_fadeout.start(i_fadeout_duration);
              }
              else {
                ms_fadeout.stop();
                b_fade_out = false;
              }
            }

            if(!b_reset_start_led && !ms_fadeout.isRunning()) {
              packOffReset();
            }
          }
        }
      break;

      case MODE_ON:
        // Turn off the status indicator LED.
        digitalWriteFast(PACK_STATUS_LED_PIN, LOW);

        if(b_spectral_lights_on) {
          spectralLightsOff();
        }

        if(b_pack_shutting_down) {
          b_pack_shutting_down = false;
        }

        if(!b_pack_on) {
          // Tell the wand the pack is on.
          packSerialSend(P_ON);
          attenuatorSend(A_PACK_ON);

          ms_fadeout.stop();
          b_fade_out = false;
          b_pack_on = true;
        }

        if(b_ramp_down && !ms_mash_lockout.isRunning()) {
          b_ramp_down = false;
          b_ramp_down_start = false;
          b_inner_ramp_down = false;

          resetRampUp();
        }

        if(ribbonCableAttached() && !b_overheating) {
          if(b_pack_alarm) {
            if(SYSTEM_YEAR == SYSTEM_1984 || SYSTEM_YEAR == SYSTEM_1989) {
              // Reset the LEDs before resetting the alarm flag.
              if(!usingSlimeCyclotron()) {
                resetCyclotronState();
              }

              ms_cyclotron.start(0);
            }
            else {
              ms_cyclotron.start(i_outer_current_ramp_speed);
            }

            ms_cyclotron_ring.start(i_inner_current_ramp_speed);

            ventLight(false);
            ventLightLEDW(false);

            b_pack_alarm = false;

            resetRampUp();

            stopEffect(S_PACK_RECOVERY);
            playEffect(S_PACK_RECOVERY);

            packStartup(false);
          }
        }

        checkCyclotronAutoSpeed();

        // Play a little bit of smoke and N-Filter vent lights while firing and other misc sound effects.
        if(b_wand_firing) {
          // Mix some impact sound effects.
          if(ms_firing_sound_mix.justFinished() && STREAM_MODE == PROTON && STATUS_CTS == CTS_NOT_FIRING && b_stream_effects) {
            uint8_t i_random = 0;

            switch(i_last_firing_effect_mix) {
              case S_FIRE_SPARKS:
                i_random = random(0,2);
              break;

              case S_FIRE_SPARKS_3:
              case S_FIRE_SPARKS_4:
                i_random = 3;
              break;

              case S_FIRE_SPARKS_5:
                i_random = 2;
              break;

              case S_FIRE_SPARKS_2:
                i_random = 1;
              break;

              default:
                // If no firing effect has played yet.
                i_random = 3;
              break;
            }

            uint16_t i_s_random = random(2,4) * 1000; // 2 or 3 seconds

            switch(i_random) {
              case 3:
                playEffect(S_FIRE_SPARKS, false, i_volume_effects, false, 0, false);
                i_last_firing_effect_mix = S_FIRE_SPARKS;

                ms_firing_sound_mix.start(i_s_random * 5);
              break;

              case 2:
                playEffect(S_FIRE_SPARKS_4, false, i_volume_effects, false, 0, false);
                i_last_firing_effect_mix = S_FIRE_SPARKS_4;

                ms_firing_sound_mix.start(i_s_random);
              break;

              case 1:
                playEffect(S_FIRE_SPARKS_3, false, i_volume_effects, false, 0, false);
                i_last_firing_effect_mix = S_FIRE_SPARKS_3;

                ms_firing_sound_mix.start(i_s_random);
              break;

              case 0:
                playEffect(S_FIRE_SPARKS_2, false, i_volume_effects, false, 0, false);
                playEffect(S_FIRE_SPARKS_5, false, i_volume_effects, false, 0, false);
                i_last_firing_effect_mix = S_FIRE_SPARKS_5;

                ms_firing_sound_mix.start(1800);
              break;

              default:
                // This will never trigger because i_random will only ever be 0~3.
                playEffect(S_FIRE_SPARKS_2, false, i_volume_effects, false, 0, false);
                i_last_firing_effect_mix = S_FIRE_SPARKS_2;

                ms_firing_sound_mix.start(500);
              break;
            }
          }

          if(ms_smoke_on.justFinished()) {
            ms_smoke_on.stop();
            ms_smoke_timer.start(PROGMEM_READU16(i_smoke_timer[i_wand_power_level - 1]));
            b_vent_sounds = true;
          }

          if(ms_smoke_timer.justFinished()) {
            if(!ms_smoke_on.isRunning()) {
              ms_smoke_on.start(PROGMEM_READU16(i_smoke_on_time[i_wand_power_level - 1]));
            }
          }

          if(ms_smoke_on.isRunning()) {
            // Turn on some smoke and play some vent sounds if smoke is enabled.
            if(b_smoke_enabled) {
              // Turn on some smoke.
              smokeNFilter(true);

              // Play some sounds with the smoke and vent lighting.
              if(b_vent_sounds) {
                playVentSounds();

                b_vent_sounds = false;
              }

              fanNFilter(true);
            }

            // We are strobing the N-Filter jewel.
            if(ms_vent_light_off.justFinished()) {
              ms_vent_light_off.stop();
              ms_vent_light_on.start(i_vent_light_delay);

              ventLight(true);
            }
            else if(ms_vent_light_on.justFinished()) {
              ms_vent_light_on.stop();
              ms_vent_light_off.start(i_vent_light_delay);

              ventLight(false);
            }

            // The LED-W will not strobe during this venting.
            ventLightLEDW(true);
          }
          else {
            smokeNFilter(false);
            ventLight(false);
            ventLightLEDW(false);
            fanNFilter(false);
          }
        }

        if(b_venting) {
          packVenting();
        }

        cyclotronControl(); // Set timers for the cyclotron.

        if(b_wand_mash_lockout && ms_mash_lockout.isRunning()) {
          if((ms_mash_lockout.delay() / 1.5) > ms_mash_lockout.remaining()) {
            // Force incorrect Powercell LED to switch it off temporarily.
            i_powercell_led = i_powercell_leds + 1;
          }
        }

        cyclotronSwitchLEDLoop(); // Update the cyclotron.

        if(b_overheating && b_overheat_lights_off) {
          powercellRampDown();
        }
        else {
          powercellLoop();
        }
      break;
    }

    switch(PACK_ACTION_STATE) {
      case ACTION_IDLE:
      default:
        // Do nothing.
      break;

      case ACTION_OFF:
        packShutdown();
      break;

      case ACTION_ACTIVATE:
        packStartup(true);
      break;
    }
  }
  else {
    systemPOST();
  }
}

void loop() {
#ifdef ESP32
  // Run checks on web-related tasks.
  webLoops();

  // Read the HDC1080 and output the current temperature reading to the debug console.
  if(b_temp_sensor_detected) {
    if(!ms_temp_read.isRunning()) {
      tempSensor.startAcquisition(GuL::HDC1080::Channel::TEMPERATURE);
      ms_temp_read.start(5000); // Read every 5 seconds
    }
    else if(ms_temp_read.justFinished()) {
      float f_temp_c = tempSensor.getTemperature();
      float f_temp_f = f_temp_c * 1.8 + 32; // Convert Celsius to Fahrenheit
      Serial.printf("\t\tTemp: %.1f C (%.1f F)\n", f_temp_c, f_temp_f);
    }
  }
#endif

  // Update the available audio device.
  updateAudio();

  // Check for any new serial commands were received from the Neutrona Wand.
  checkWand();

  // Check if the wand is considered to have been disconnected.
  wandDisconnectCheck();

  // Check if Attenuator is present.
  attenuatorHandShake();

  // Check if any new serial commands were received.
  checkAttenuator();

  // Handle any actions after POST event.
  mainLoop();

  // Update the LEDs
  if(ms_fast_led.justFinished()) {
    FastLED.show();

    ms_fast_led.start(i_fast_led_delay);

    if(b_powercell_updating) {
      b_powercell_updating = false;
    }
  }
}
