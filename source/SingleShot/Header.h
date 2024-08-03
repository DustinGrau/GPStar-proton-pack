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

/*
 * Device state.
 */
enum DEVICE_STATE { MODE_OFF, MODE_ON, MODE_ERROR };
enum DEVICE_STATE DEVICE_STATUS;

/*
 * Various device action states.
 */
enum DEVICE_ACTION_STATE { ACTION_IDLE, ACTION_OFF, ACTION_ACTIVATE, ACTION_FIRING, ACTION_SETTINGS, ACTION_ERROR, ACTION_CONFIG_EEPROM_MENU };
enum DEVICE_ACTION_STATE DEVICE_ACTION_STATUS;

/*
 * Device Stream Modes + Settings
 * Stream = Type of particle stream to be thrown by the device
 */
enum STREAM_MODES { PROTON };
enum STREAM_MODES STREAM_MODE;
enum POWER_LEVELS { LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, LEVEL_5 };
enum POWER_LEVELS POWER_LEVEL;
enum POWER_LEVELS POWER_LEVEL_PREV;

/*
 * Addressable LEDs
 * The device contains a mini cyclotron plus a barrel light. A simple NeoPixel Jewel can be used
 * for the cyclotron (typically 7 LEDs) while the barrel is designed to use the GPStar single LED.
 */
#define SYSTEM_LED_PIN 10
#define CYCLOTRON_LED_COUNT 7 // NeoPixel Jewel
#define BARREL_LED_COUNT 1 // GPStar Barrel LED
CRGB system_leds[CYCLOTRON_LED_COUNT + BARREL_LED_COUNT];
const uint8_t i_barrel_led = CYCLOTRON_LED_COUNT; // This will be the index of the light, not the count
const uint8_t i_num_cyclotron_leds = CYCLOTRON_LED_COUNT; // This will be the number of cyclotron LEDs (jewel)

/*
 * Mini Cyclotron
 * Alternates between a pair of LEDs in the jewel, fading in by some number of steps per update of the timer.
 */
millisDelay ms_cyclotron;
const uint8_t i_cyclotron_leds[i_num_cyclotron_leds] = {0, 1, 2, 3, 4, 5, 6}; // Note: 0 is the dead center of the jewel
const uint8_t i_cyclotron_max_steps = 12; // Set a reusable constant for the maximum number of steps to cycle through
// Sequence: 1, 4, 2, 5, 3, 6, 4, 1, 5, 2, 6, 3
const uint8_t i_cyclotron_pair[i_cyclotron_max_steps][2] = {
  {1, 3}, // 1:in, 3:out,
  {1, 4}, // 1:out, 4:in,
  {2, 4}, // 2:in, 4:out,
  {2, 5}, // 2:out, 5:in,
  {3, 5}, // 3:in, 5:out,
  {3, 6}, // 3:out, 6:in,
  {4, 6}, // 4:in, 6:out,
  {4, 1}, // 4:out, 1:in,
  {5, 1}, // 5:in, 1:out,
  {5, 2}, // 5:out, 2:in,
  {6, 2}, // 6:in, 2:out,
  {6, 3}  // 6:out, 3:in,
};
const uint16_t i_base_cyclotron_delay = 30; // Set delay between LED updates at normal speed, at the lowest power level
const uint16_t i_min_cyclotron_delay = 10;  // Set the minimum (fastest) transition time desired for a cyclotron update
const uint8_t i_cyc_fade_step = 15; // Step size for each fade-in increment (must be a divisor of 255: 3, 5, 15, 17, 51, 85)
const uint8_t i_cyclotron_min_brightness = 0;   // Minimum brightness for each LED (use fade step for changes)
const uint8_t i_cyclotron_max_brightness = 255; // Maximum brightness for each LED (use fade step for changes)

/*
 * Delay for fastled to update the addressable LEDs.
 * 0.03 ms to update 1 LED. So 1.47 ms should be okay? Let's bump it up to 3 just in case.
 */
#define FAST_LED_UPDATE_MS 3
uint8_t i_fast_led_delay = FAST_LED_UPDATE_MS; // Default delay via standard definition
millisDelay ms_fast_led; // Timer for all updates to addressable LEDs across the device

/*
 * Non-addressable LEDs
 * Uses a common object to define and set expected properties for all LEDs
 */
struct SimpleLED {
  uint8_t Pin; // Pin Assignment
  uint8_t On;  // State for "on"
  uint8_t Off; // State for "off"

  // Function to initialize the LED
  void initialize() {
      pinModeFast(Pin, OUTPUT);
      digitalWriteFast(Pin, Off);
  }

  // Function to dim the LED
  void dim(uint8_t brightness) {
      analogWrite(Pin, brightness);
  }

  // Function to get LED state
  uint8_t state() {
      return digitalReadFast(Pin);
  }

  // Function to turn on the LED
  void turnOn() {
      digitalWriteFast(Pin, On);
  }

  // Function to turn off the LED
  void turnOff() {
      digitalWriteFast(Pin, Off);
  }
};
// Create instances and initialize LEDs
SimpleLED led_SloBlo = {8, HIGH, LOW};
SimpleLED led_Clippard = {9, HIGH, LOW};
SimpleLED led_TopWhite = {12, LOW, HIGH};
SimpleLED led_Vent = {13, LOW, HIGH};
SimpleLED led_Hat1 = {22, HIGH, LOW};
SimpleLED led_Hat2 = {23, HIGH, LOW};
SimpleLED led_Tip = {24, HIGH, LOW};

/*
 * Rotary encoder on the top of the device.
 * Changes the device power level and controls the device settings menu.
 * Also controls independent music volume while the device is off and if music is playing.
 */
#define r_encoderA 6
#define r_encoderB 7
enum ENCODER_STATES { ENCODER_IDLE, ENCODER_CW, ENCODER_CCW };
struct Encoder {
  const static uint8_t PinA = r_encoderA;
  const static uint8_t PinB = r_encoderB;

  private:
    uint8_t PrevNextCode = 0;
    uint16_t CodeStore = 0;

    int8_t read() {
      static int8_t RotEncTable[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};

      PrevNextCode <<= 2;

      if(digitalReadFast(r_encoderB)) {
        PrevNextCode |= 0x02;
      }

      if(digitalReadFast(r_encoderA)) {
        PrevNextCode |= 0x01;
      }

      PrevNextCode &= 0x0f;

      // If valid then CodeStore as 16 bit data.
      if(RotEncTable[PrevNextCode]) {
          CodeStore <<= 4;
          CodeStore |= PrevNextCode;

          if((CodeStore & 0xff) == 0x2b) {
            return -1;
          }

          if((CodeStore & 0xff) == 0x17) {
            return 1;
          }
      }

      return 0;
    }

  public:
    enum ENCODER_STATES STATE;

    void initialize() {
      // Rotary encoder on the top of the device.
      pinModeFast(PinA, INPUT_PULLUP);
      pinModeFast(PinB, INPUT_PULLUP);
      STATE = ENCODER_IDLE;
    }

    void check() {
      static int8_t i_last_val; // Always checked to know if change occurred.

      // Read the current encoder value, noting state if adjusted.
      if(i_last_val != read()) {
        // Clockwise.
        if(PrevNextCode == 0x07) {
          STATE = ENCODER_CW;
          debugln("CW");
        }

        // Counter-clockwise.
        if(PrevNextCode == 0x0b) {
          STATE = ENCODER_CCW;
          debugln("CCW");
        }
      }
      else {
        STATE = ENCODER_IDLE;
      }
    }

} encoder;

/*
 * Vibration
 *
 * These are references for the EEPROM menu. Empty is a zero value, not used in the EEPROM.
 */
enum VIBRATION_MODES_EEPROM { VIBRATION_EMPTY, VIBRATION_ALWAYS, VIBRATION_FIRING_ONLY, VIBRATION_NONE };
enum VIBRATION_MODES_EEPROM VIBRATION_MODE_EEPROM;
const uint8_t vibration = 11;
const uint8_t i_vibration_level_min = 65;
uint8_t i_vibration_level = i_vibration_level_min;
uint8_t i_vibration_level_prev = 0;
millisDelay ms_menu_vibration; // Timer to do non-blocking confirmation buzzing in the vibration menu.

/*
 * Various Switches on the device.
 */
Switch switch_intensify(2); // Considered a primary firing button, though for this device will be an alt-fire.
Switch switch_activate(3); // Considered the primary power toggle on the right of the gun box.
Switch switch_device(A0); // Controls the beeping. Top right switch on the device.
Switch switch_vent(4); // Turns on the vent light. Bottom right switch on the device.
Switch switch_grip(A6); // Hand-grip button to be the primary fire and used in settings menus.
bool b_all_switch_activation = false; // Used to check if Activate was flipped to on while the vent switch was already in the on position for sound purposes.
uint8_t ventSwitchedCount = 0;
uint8_t deviceSwitchedCount = 0;

// Used to scan the i2c bus and to locate the 28-segment bargraph.
#define WIRE Wire

/*
 * Barmeter 28 segment bargraph configuration and timers.
 * Part #: BL28Z-3005SA04Y
 * This will use the following pins for i2c serial communication:
 * Arduino Nano
 *   SDA -> A4
 *   SCL -> A5
 * ESP32
 *   SDA -> GPIO 21
 *   SCL -> GPIO 22
 */
HT16K33 ht_bargraph;
const uint8_t i_bargraph_delay = 8; // Base delay (ms) for bargraph refresh (this should be a value evenly divisible by 2, 3, or 4).
const uint8_t i_bargraph_elements = 28; // Maximum elements for bargraph device; not likely to change but adjustable just in case.
const uint8_t i_bargraph_levels = 5; // Reflects the count of POWER_LEVELS elements (the only dependency on other device behavior).
uint8_t i_bargraph_sim_max = i_bargraph_elements; // Simulated maximum for patterns which may be dependent on other factors.
uint8_t i_bargraph_steps = i_bargraph_elements / 2; // Steps for patterns (1/2 max) which are bilateral/mirrored.
uint8_t i_bargraph_step = 0; // Indicates current step for bilateral/mirrored patterns.
int i_bargraph_element = 0; // Indicates current LED element for adjustment.
bool b_bargraph_present = false; // Denotes that i2c bus found the bargraph device.
millisDelay ms_bargraph; // Timer to control bargraph updates consistently.

/*
 * Barmeter 28 segment bargraph mapping: allows accessing elements sequentially (0-27)
 * If the pattern appears inverted from what is expected, flip by using the following:
 */
//#define GPSTAR_INVERT_BARGRAPH
#ifdef GPSTAR_INVERT_BARGRAPH
  const uint8_t i_bargraph[28] = {54, 38, 22, 6, 53, 37, 21, 5, 52, 36, 20, 4, 51, 35, 19, 3, 50, 34, 18, 2, 49, 33, 17, 1, 48, 32, 16, 0};
#else
  const uint8_t i_bargraph[28] = {0, 16, 32, 48, 1, 17, 33, 49, 2, 18, 34, 50, 3, 19, 35, 51, 4, 20, 36, 52, 5, 21, 37, 53, 6, 22, 38, 54};
#endif

/*
 * Control for the primary blast sound effects.
 */
millisDelay ms_single_blast;
const uint16_t i_single_blast_delay_level_5 = 240;
const uint16_t i_single_blast_delay_level_4 = 260;
const uint16_t i_single_blast_delay_level_3 = 280;
const uint16_t i_single_blast_delay_level_2 = 300;
const uint16_t i_single_blast_delay_level_1 = 320;

/*
 * Idling timers
 */
millisDelay ms_white_light;
const uint16_t i_top_blink_interval = 146; // Blinking interval (ms)

/*
 * For blinking the slo-blo light when the cyclotron is not on.
 */
millisDelay ms_slo_blo_blink;
const uint16_t i_slo_blo_blink_delay = 500;

/*
 * Timer for rotary firing mode select speed limit (delay when switching firing modes).
 */
millisDelay ms_firing_mode_switch;
const uint8_t i_firing_mode_switch_delay = 50;

/*
 * Timers for the optional hat lights.
 * Also used for vent lights during error modes.
 */
millisDelay ms_hat_1;
millisDelay ms_hat_2;
const uint8_t i_hat_1_delay = 100;
const uint16_t i_hat_2_delay = 400;

/*
 * Device tip heatup timers (when changing firing modes).
 */
millisDelay ms_device_heatup_fade;
const uint8_t i_delay_heatup = 5;
uint8_t i_heatup_counter = 0;
uint8_t i_heatdown_counter = 100;

/*
 * Firing timers.
 */
millisDelay ms_firing_lights;
millisDelay ms_firing_lights_end;
millisDelay ms_firing_effect_end;
millisDelay ms_firing_stream_effects;
millisDelay ms_firing_pulse;
millisDelay ms_impact; // Mix some impact sounds while firing.
millisDelay ms_firing_length_timer;
millisDelay ms_semi_automatic_check; // Timer used to set the rate of fire for the semi-automatic firing modes.
millisDelay ms_semi_automatic_firing; // Timer used to handle firing effect duration for the semi-automatic firing modes.
const uint16_t i_single_shot_rate = 2000; // Single shot firing rate, locking out actions after each blast.
const uint16_t i_firing_timer_length = 15000; // 15 seconds. Used by ms_firing_length_timer to determine which tail_end sound effects to play.
const uint8_t i_firing_pulse = 60; // Used to drive semi-automatic firing stream effect timers.
const uint8_t i_firing_stream = 100; // Used to drive all stream effects timers. Default: 100ms.
const uint8_t i_pulse_step_max = 8; // Total number of steps per pulse animation.
uint8_t i_pulse_step = 0; // Used to keep track of which pulse animation step we are on.
uint16_t i_last_firing_effect_mix = 0; // Used by standalone Single-Shot Blaster.

/*
 * Device Menu
 */
enum DEVICE_MENU_LEVELS { MENU_LEVEL_1, MENU_LEVEL_2, MENU_LEVEL_3, MENU_LEVEL_4, MENU_LEVEL_5 };
enum DEVICE_MENU_LEVELS DEVICE_MENU_LEVEL;
uint8_t i_device_menu = 5;
const uint16_t i_settings_blinking_delay = 350;
millisDelay ms_settings_blinking;

/*
 * Misc device settings and flags.
 */
bool b_firing = false; // Check for general firing state.
bool b_firing_intensify = false; // Check for Intensify button activity.
bool b_firing_alt = false; // Check for grip button firing activity.
bool b_firing_semi_automatic = false; // Check for semi-automatic firing modes.
bool b_sound_firing_intensify_trigger = false;
bool b_sound_firing_alt_trigger = false;
bool b_sound_firing_cross_the_streams = false;
bool b_sound_idle = false;
bool b_beeping = false;
bool b_sound_afterlife_idle_2_fade = true;
bool b_device_boot_error_on = false;

/*
 * Button Mashing Lock-out - Prevents excessive user input via the primary/secondary firing buttons.
 * This ensures that the user is not exceeding what would be considered "normal" for firing of the device,
 * otherwise an error mode will be engaged to provide a cool-down period. This does not apply to any
 * prolonged firing which would trigger the overheat or venting sequences; only rapid firing bursts.
 */
millisDelay ms_bmash;              // Timer for the button mash lock-out period.
uint16_t i_bmash_delay = 2000;     // Time period in which we consider rapid firing.
uint16_t i_bmash_cool_down = 3000; // Time period for the lock-out of user input.
uint8_t i_bmash_count = 0;         // Current count for rapid firing bursts.
uint8_t i_bmash_max = 7;           // Burst count we consider before the lock-out.
uint8_t i_bmash_spark_index = 0;   // Current spark number for the spark effect (0~2).
bool b_device_mash_error = false;    // Indicates if device is in a lock-out phase.

/*
 * Used during the overheating sequences.
*/
millisDelay ms_blink_sound_timer_1;
millisDelay ms_blink_sound_timer_2;
const uint16_t i_blink_sound_timer = 400;

/*
 * A timer to turn on some Single-Shot Blaster lights when the system is shut down after some inactivity, as a reminder you left your power on to the system.
*/
millisDelay ms_power_indicator;
millisDelay ms_power_indicator_blink;
const uint32_t i_ms_power_indicator = 60000; // 1 Minute -> 60000
const uint16_t i_ms_power_indicator_blink = 1000;

/*
 * Function prototypes.
 */
void checkDeviceAction();
void ventSwitched(void* n = nullptr);
void deviceSwitched(void* n = nullptr);
