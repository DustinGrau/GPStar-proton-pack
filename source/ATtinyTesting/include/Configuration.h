#pragma once

/*
 * You can set the default master startup volume for your pack here.
 * When a Neutrona Wand is connected, it will sync to these settings.
 * Values are in % of the volume.
 * 0 = quietest
 * 100 = loudest
 */
const uint8_t STARTUP_VOLUME = 100;

/*
 * You can set the default music volume for your pack here.
 * When a Neutrona Wand is connected, it will sync to these settings.
 * Values are in % of the volume.
 * 0 = quietest
 * 100 = loudest
 */
const uint8_t STARTUP_VOLUME_MUSIC = 100;

/*
 * You can set the default sound effects volume for your pack here.
 * When a Neutrona Wand is connected, it will sync to these settings.
 * Values are in % of the volume.
 * 0 = quietest
 * 100 = loudest
 */
const uint8_t STARTUP_VOLUME_EFFECTS = 100;

/*
 * Minimum volume that the pack can achieve.
 * Values must be from 0 to -70. 0 = the loudest and -70 = the quietest (no audible sound).
 * Volume changes are based on percentages which are converted to the appropriate decibel value.
 * If your pack is overpowering the wand at lower volumes, you can either increase the minimum value in the wand,
 * or decrease the minimum value for the pack. By default the pack will be nearly silent at 0% volume, but not off.
 */
const int8_t MINIMUM_VOLUME = -60;

/*
 * Percentage increments of main volume change.
 */
const uint8_t VOLUME_MULTIPLIER = 5;

/*
 * Percentage increments of the music volume change.
 */
const uint8_t VOLUME_MUSIC_MULTIPLIER = 5;

/*
 * Percentage increments of the sound effects volume change.
 */
const uint8_t VOLUME_EFFECTS_MULTIPLIER = 5;

/*
 * Set to true to enable the onboard amplifier on the WAV Trigger.
 * This is for the WAV Trigger only and does not affect GPStar Audio.
 * If you use the output pins directly on the WAV Trigger board to your speakers, you will need to enable the onboard amp.
 * NOTE: The onboard mono audio amplifier and speaker connector specifications: 2W into 4 Ohms, 1.25W into 8 Ohms
 */
const bool b_onboard_amp_enabled = false;
