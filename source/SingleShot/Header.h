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
 * Bargraph modes.
 * Super Hero: Mimics the super hero bargraph animations from the Single-Shot Blaster closeup in the 1984 rooftop. This is the default for 1984/1989 and Super Hero Mode.
 * Original: Mimics the original diagrams and instructions based on production notes and in Afterlife. This is the default for Afterlife and Mode Original.
 */
enum BARGRAPH_MODES { BARGRAPH_SUPER_HERO };
enum BARGRAPH_MODES BARGRAPH_MODE;

/*
 * Bargraph Firing Animations.
 * Animation Super Hero: Mimics the fandom animations of the bargraph scrolling up and down with 2 lines with it merging in the middle. This is the default for 1984/1989 and Super Hero Mode.
 * Animation Original: Mimics the original diagrams and instructions based on production notes. This is the default for Afterlife and Mode Original.
 */
enum BARGRAPH_FIRING_ANIMATIONS { BARGRAPH_ANIMATION_SUPER_HERO, BARGRAPH_ANIMATION_ORIGINAL };
enum BARGRAPH_FIRING_ANIMATIONS BARGRAPH_FIRING_ANIMATION;

/*
 * For MODE_ORIGINAL. For blinking the slo-blo light when the cyclotron is not on.
 */
millisDelay ms_slo_blo_blink;
const uint16_t i_slo_blo_blink_delay = 500;

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
const uint8_t i_cyclotron_pair[2] = {3, 6}; // Choice of opposites to match with installation: 1/4, 2/5, 3/6
const uint16_t i_base_cyclotron_delay = 40; // Set delay between LED updates at normal speed, at the lowest power level
const uint16_t i_min_cyclotron_delay = 20; // Set the minimum (fastest) transition time desired for a cyclotron update
const uint8_t i_cyclotron_max_brightness = 255; // Maximum brightness
const uint8_t i_cyc_fade_step = 5; // Step size for each fade-in increment

/*
 * Control for the primary blast sound effects.
 */
millisDelay ms_single_blast;
const uint16_t i_single_blast_delay_level_5 = 340;
const uint16_t i_single_blast_delay_level_4 = 360;
const uint16_t i_single_blast_delay_level_3 = 380;
const uint16_t i_single_blast_delay_level_2 = 400;
const uint16_t i_single_blast_delay_level_1 = 420;

/*
 * Delay for fastled to update the addressable LEDs.
 * 0.03 ms to update 1 LED. So 1.47 ms should be okay? Let's bump it up to 3 just in case.
 */
#define FAST_LED_UPDATE_MS 3
uint8_t i_fast_led_delay = FAST_LED_UPDATE_MS;
millisDelay ms_fast_led;

/*
 * Non-addressable LEDs
 */
const uint8_t led_slo_blo = 8; // SLO-BLO LED. (Red LED)
const uint8_t led_front_left = 9; // LED underneath the Clippard valve. (Orange or White LED)
const uint8_t led_white = 12; // Blinking white light beside the vent on top of the device.
const uint8_t led_vent = 13; // Vent light
const uint8_t led_hat_1 = 22; // Hat light at front of the device near the barrel tip. (Orange LED)
const uint8_t led_hat_2 = 23; // Hat light at top of the device body near vent. (Orange or White LED)
const uint8_t led_barrel_tip = 24; // White LED at tip of the device barrel. (White LED)

/*
 * Time in milliseconds for blinking the top white LED while the device is on.
 */
const uint16_t i_top_blink_interval = 146;

/*
 * Rotary encoder on the top of the device. Changes the device power level and controls the device settings menu.
 * Also controls independent music volume while the device is off and if music is playing.
 */
#define r_encoderA 6
#define r_encoderB 7
millisDelay ms_firing_mode_switch; // Timer for rotary firing mode select speed limit.
const uint8_t i_firing_mode_switch_delay = 50; // Time to delay switching firing modes.
static uint8_t prev_next_code = 0;
static uint16_t store = 0;

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

/*
 * Idling timers
 */
millisDelay ms_white_light;

/*
 * Barmeter 28-segment bargraph configuration and timers.
 * Part #: BL28Z-3005SA04Y
 */
HT16K33 ht_bargraph;

// Used to scan the i2c bus and to locate the 28-segment bargraph.
#define WIRE Wire

/*
 * Bargraph Timers
 */
millisDelay ms_bargraph;
millisDelay ms_bargraph_firing;
const uint8_t d_bargraph_ramp_interval = 120;
uint8_t i_bargraph_status = 0;

/*
 * Used to change to 28-segment bargraph features.
 * The Frutto 28-segment bargraph is automatically detected on boot and sets this to true.
 * Part #: BL28Z-3005SA04Y
 */
bool b_28segment_bargraph = true;
const uint8_t i_bargraph_interval = 4;
const uint8_t i_bargraph_wait = 180;
bool b_bargraph_up = false;
millisDelay ms_bargraph_alt;
uint8_t i_bargraph_status_alt = 0;
const uint8_t d_bargraph_ramp_interval_alt = 40;
const uint8_t i_bargraph_multiplier_ramp_2021 = 16;
uint16_t i_bargraph_multiplier_current = i_bargraph_multiplier_ramp_2021;

/*
 * (Optional) Barmeter 28-segment bargraph mapping.
 * Part #: BL28Z-3005SA04Y

 * Segment Layout:
 * 5: full: 23 - 27  (5 segments)
 * 4: 3/4: 17 - 22   (6 segments)
 * 3: 1/2: 12 - 16   (5 segments)
 * 2: 1/4: 5 - 11    (7 segments)
 * 1: none: 0 - 4    (5 segments)
 */
const uint8_t i_bargraph_segments = 28;
const uint8_t i_bargraph_invert[i_bargraph_segments] PROGMEM = {54, 38, 22, 6, 53, 37, 21, 5, 52, 36, 20, 4, 51, 35, 19, 3, 50, 34, 18, 2, 49, 33, 17, 1, 48, 32, 16, 0};
const uint8_t i_bargraph_normal[i_bargraph_segments] PROGMEM = {0, 16, 32, 48, 1, 17, 33, 49, 2, 18, 34, 50, 3, 19, 35, 51, 4, 20, 36, 52, 5, 21, 37, 53, 6, 22, 38, 54};
bool b_bargraph_status[i_bargraph_segments] = {};

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
 * Device Stream Modes + Settings
 * Stream = Type of particle stream to be thrown by the device
 */
enum STREAM_MODES { PROTON };
enum STREAM_MODES STREAM_MODE;

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
millisDelay ms_firing_sound_mix; // Mix additional impact sounds for standalone Single-Shot Blaster.
millisDelay ms_semi_automatic_check; // Timer used to set the rate of fire for the semi-automatic firing modes.
millisDelay ms_semi_automatic_firing; // Timer used to handle firing effect duration for the semi-automatic firing modes.
const uint16_t i_single_shot_rate = 2000; // Single shot firing rate.
const uint16_t i_firing_timer_length = 15000; // 15 seconds. Used by ms_firing_length_timer to determine which tail_end sound effects to play.
const uint8_t d_firing_pulse = 18; // Used to drive semi-automatic firing stream effect timers. Default: 18ms.
const uint8_t d_firing_stream = 100; // Used to drive all stream effects timers. Default: 100ms.
uint8_t i_cyclotron_light = 0; // Used to keep track which LED in the cyclotron is currently lighting up.
const uint8_t i_pulse_step_max = 6; // Total number of steps per pulse animation.
uint8_t i_pulse_step = 0; // Used to keep track of which pulse animation step we are on.
uint16_t i_last_firing_effect_mix = 0; // Used by standalone Single-Shot Blaster.

/*
 * Device power level. Controlled by the rotary encoder on the top of the device.
 * You can enable or disable overheating for each power level individually in the user adjustable values at the top of this file.
 */
const uint8_t i_power_level_max = 5;
const uint8_t i_power_level_min = 1;
uint8_t i_power_level = 5;
uint8_t i_power_level_prev = 5;

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
bool b_firing_alt = false; // Check for Barrel Wing Button firing activity for CTS.
bool b_firing_semi_automatic = false; // Check for semi-automatic firing modes.
bool b_sound_firing_intensify_trigger = false;
bool b_sound_firing_alt_trigger = false;
bool b_sound_firing_cross_the_streams = false;
bool b_sound_idle = false;
bool b_beeping = false;
bool b_sound_afterlife_idle_2_fade = true;
bool b_device_boot_error_on = false;

/*
 * Mini cyclotron flags and values.
 */
uint8_t i_cyclotron_speed_up = 1; // For telling the device to speed up or slow down the Cyclotron lights.

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
