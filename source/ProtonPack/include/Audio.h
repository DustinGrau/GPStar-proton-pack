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

#pragma once

/**
 ***** IMPORTANT *****
 * If using a WAV Trigger, please make sure they are running the custom GPStar version firmware version 1.40 or higher.
 * You can download the latest directly from the GPStar github repository.
 * https://github.com/gpstar81/haslab-proton-pack/tree/main/extras
 *
 * Information on how to update your WAV Trigger devices can be found on the GPStar github repository.
 * https://github.com/gpstar81/haslab-proton-pack/blob/main/WAVTRIGGER.md
 */
#include <GPStarAudio.h>
gpstarAudio audio;

// --- AudioSerial definition for ESP32 ---
// The ESP32 macro is automatically defined by the Arduino/PlatformIO toolchain
// when compiling for ESP32-based boards. No need to define it manually.
// HardwareSerial is provided by the ESP32 Arduino core and allows creation of
// additional UART serial ports. See: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/serial.html
#ifdef ESP32
  #ifndef AUDIO_RX_PIN
    #define AUDIO_RX_PIN 15
  #endif
  #ifndef AUDIO_TX_PIN
    #define AUDIO_TX_PIN 16
  #endif
  // Create a HardwareSerial instance for AudioSerial set to UART2.
  HardwareSerial AudioSerial(2);
#else
  // On Mega 2560, alias AudioSerial to Serial3 instead.
  #define AudioSerial Serial3
#endif

/*
 * Audio Devices
 */
enum AUDIO_DEVICES { A_NONE, A_GPSTAR_AUDIO, A_GPSTAR_AUDIO_ADV, A_WAV_TRIGGER };
enum AUDIO_DEVICES AUDIO_DEVICE = A_NONE;

/*
 * Audio Variables
 */
uint16_t i_music_count = 0; // Contains the total number of detected music tracks on the SD card.
uint16_t i_current_music_track = 0; // Sets the ID number for the music track to be played.
const uint16_t i_music_track_start = 500; // Music tracks start on file named 500_ and higher.
const int8_t i_volume_abs_min = -70; // System (absolute) minimum volume possible.
const int8_t i_volume_abs_max = 0; // System (absolute) maximum volume possible.
uint8_t i_volume_min_adj = 0; // Adjustment factor for minimum volume. 0 for WAV Trigger, 10 for GPStar Audio.
const uint8_t i_wand_idle_level = 20; // This adjusts the volume of certain Afterlife / Frozen Empire Neutrona Wand idle sounds that the Proton pack can play.
bool b_playing_music = false; // Sets whether a music track is currently playing or not.
bool b_music_paused = false; // Sets whether a music track is currently paused or not.
bool b_repeat_track = false; // Sets whether to repeat one music track or loop through all music tracks.
bool b_preload_tracks = false; // Sets whether to add a 50ms delay before playing any file to allow slower SD cards more time to fill the buffer.

/*
 * Music Control/Checking
 */
const uint16_t i_music_check_delay = 2000;
const uint16_t i_music_next_track_delay = 500;
millisDelay ms_check_music;
millisDelay ms_music_next_track;
millisDelay ms_music_status_check;

/*
 * Volume percentage values (0 to 100)
 */
uint8_t i_volume_master_percentage = STARTUP_VOLUME; // Master overall volume.
uint8_t i_volume_effects_percentage = STARTUP_VOLUME_EFFECTS; // Sound effects.
uint8_t i_volume_music_percentage = STARTUP_VOLUME_MUSIC; // Music volume.

/*
 * General Volume
 * Master Volume: (MINIMUM_VOLUME + i_volume_min_adj) = Quietest, i_volume_abs_max = Loudest
 * Effects/Music: i_volume_abs_min = Quietest, i_volume_abs_max = Loudest
 */
int8_t i_volume_master = (MINIMUM_VOLUME + i_volume_min_adj) - ((MINIMUM_VOLUME + i_volume_min_adj) * i_volume_master_percentage / 100); // Master overall volume.
int8_t i_volume_master_eeprom = i_volume_master; // Master overall volume that is saved into the eeprom menu and loaded during bootup.
int8_t i_volume_revert = i_volume_master; // Used to restore volume level from a muted state.
int8_t i_volume_effects = i_volume_abs_min - (i_volume_abs_min * i_volume_effects_percentage / 100); // Sound effects.
int8_t i_volume_music = i_volume_abs_min - (i_volume_abs_min * i_volume_music_percentage / 100); // Music volume.

/*
 * Function Prototypes
 */
void playEffect(uint16_t i_track_id, bool b_track_loop = false, int8_t i_track_volume = i_volume_effects, bool b_fade_in = false, uint16_t i_fade_time = 0, bool b_lock = true);
void stopEffect(uint16_t i_track_id);
void playTransitionEffect(uint16_t i_track_id, uint16_t i_track_id2, bool b_track2_loop = false, uint16_t i_track2_offset = 0, int8_t i_track_volume = i_volume_effects, bool b_fade_in = false, uint16_t i_fade_time = 0, bool b_lock = true);
void adjustGainEffect(uint16_t i_track_id, int8_t i_track_volume = i_volume_effects, bool b_fade = false, uint16_t i_fade_time = 0);
void updateMasterVolume(bool startup = false);

/*
 * Audio playback functions.
 */

// Play a sound effect using certain defaults.
void playEffect(uint16_t i_track_id, bool b_track_loop, int8_t i_track_volume, bool b_fade_in, uint16_t i_fade_time, bool b_lock) {
  if(i_track_volume < i_volume_abs_min) {
    i_track_volume = i_volume_abs_min;
  }

  if(i_track_volume > i_volume_abs_max) {
    i_track_volume = i_volume_abs_max;
  }

  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      if(b_fade_in) {
        audio.trackGain(i_track_id, i_volume_abs_min);
        audio.trackPlayPoly(i_track_id, b_lock);
        audio.trackFade(i_track_id, i_track_volume, i_fade_time, 0);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock);
      }

      if(b_track_loop) {
        audio.trackLoop(i_track_id, 1);
      }
      else {
        audio.trackLoop(i_track_id, 0);
      }
    break;

    case A_GPSTAR_AUDIO_ADV:
      if(b_fade_in) {
        audio.trackGain(i_track_id, i_volume_abs_min);
        audio.trackPlayPoly(i_track_id, b_lock, b_preload_tracks ? 50 : 0);
        audio.trackFade(i_track_id, i_track_volume, i_fade_time, 0);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock, b_preload_tracks ? 50 : 0);
      }

      if(b_track_loop) {
        audio.trackLoop(i_track_id, 1);
      }
      else {
        audio.trackLoop(i_track_id, 0);
      }
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

void stopEffect(uint16_t i_track_id) {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.trackStop(i_track_id);
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

// Play a sound effect that plays a second sound effect once complete.
void playTransitionEffect(uint16_t i_track_id, uint16_t i_track_id2, bool b_track2_loop, uint16_t i_track2_offset, int8_t i_track_volume, bool b_fade_in, uint16_t i_fade_time, bool b_lock) {
  if(i_track_volume < i_volume_abs_min) {
    i_track_volume = i_volume_abs_min;
  }

  if(i_track_volume > i_volume_abs_max) {
    i_track_volume = i_volume_abs_max;
  }

  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO_ADV:
      if(b_fade_in) {
        audio.trackGain(i_track_id, i_volume_abs_min);
        audio.trackGain(i_track_id2, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock, b_preload_tracks ? 50 : 0, i_track_id2, b_track2_loop, i_track2_offset);
        audio.trackFade(i_track_id, i_track_volume, i_fade_time, 0);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
        audio.trackGain(i_track_id2, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock, b_preload_tracks ? 50 : 0, i_track_id2, b_track2_loop, i_track2_offset);
      }
    break;

    default:
      // No valid audio device connected.
    break;
  }
}

// Play a music track using certain defaults.
void playMusic() {
  if(i_music_count > 0 && i_current_music_track >= i_music_track_start) {
    b_playing_music = true;

    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
        // Loop the music track.
        if(b_repeat_track) {
          audio.trackLoop(i_current_music_track, 1);
        }
        else {
          audio.trackLoop(i_current_music_track, 0);
        }

        audio.trackGain(i_current_music_track, i_volume_music);
        audio.trackPlayPoly(i_current_music_track, true);
        audio.update();

        audio.resetTrackCounter();
      break;

      case A_GPSTAR_AUDIO_ADV:
        // Loop the music track.
        if(b_repeat_track) {
          audio.trackLoop(i_current_music_track, 1);
        }
        else {
          audio.trackLoop(i_current_music_track, 0);
        }

        audio.trackGain(i_current_music_track, i_volume_music);
        audio.trackPlayPoly(i_current_music_track, true, b_preload_tracks ? 50 : 0);
        audio.update();

        audio.resetTrackCounter();
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    // Manage track navigation.
    ms_music_status_check.start(i_music_check_delay * 5);

    // Tell connected wand that music playback has started.
    packSerialSend(P_MUSIC_STATUS, b_playing_music ? 2 : 1);

    // Tell connected serial device music playback has started.
    attenuatorSend(A_MUSIC_IS_PLAYING, i_current_music_track);
  }
}

void stopMusic() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      if(i_music_count > 0 && i_current_music_track >= i_music_track_start) {
        audio.trackStop(i_current_music_track);
      }

      audio.update();
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }

  b_music_paused = false;
  b_playing_music = false;

  // Tell connected wand that music playback has started.
  packSerialSend(P_MUSIC_STATUS, b_playing_music ? 2 : 1);

  // Tell connected serial device music playback has stopped.
  attenuatorSend(A_MUSIC_IS_NOT_PLAYING, i_current_music_track);
}

void pauseMusic() {
  if(b_playing_music && !b_music_paused) {
    // Stop the music check timer.
    ms_music_status_check.stop();

    // Pause music playback on the Proton Pack
    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
      case A_GPSTAR_AUDIO_ADV:
        audio.trackPause(i_current_music_track);
        audio.update();
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    b_music_paused = true;

    // Tell connected wand that music playback is paused.
    packSerialSend(P_MUSIC_STATUS, b_music_paused ? 4 : 3);

    // Tell connected devices music playback is paused.
    attenuatorSend(A_MUSIC_IS_PAUSED);
  }
}

void resumeMusic() {
  if(b_music_paused) {
    // Reset the music check timer.
    ms_music_status_check.start(i_music_check_delay * 4);

    // Resume music playback on the Proton Pack
    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
      case A_GPSTAR_AUDIO_ADV:
        audio.resetTrackCounter();
        audio.trackResume(i_current_music_track);
        audio.update();
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    b_music_paused = false;

    // Tell connected wand that music playback is paused.
    packSerialSend(P_MUSIC_STATUS, b_music_paused ? 4 : 3);

    // Tell connected devices music playback has resumed.
    attenuatorSend(A_MUSIC_IS_NOT_PAUSED);
  }
}

void musicNextTrack() {
  uint16_t i_temp_track = i_current_music_track; // Used for music navigation.

  // Determine the next track.
  if(i_current_music_track + 1 > i_music_track_start + i_music_count - 1) {
    // Start at the first track if already on the last.
    i_temp_track = i_music_track_start;
  }
  else {
    i_temp_track++;
  }

  // Switch to the next track.
  if(b_playing_music) {
    // Stops music using the current track number as the identifier.
    stopMusic();

    i_current_music_track = i_temp_track; // Change only AFTER stopping music playback.

    // Play the appropriate track on pack and wand, and notify the Attenuator.
    playMusic();
  }
  else {
    // Set the new track.
    i_current_music_track = i_temp_track;

    attenuatorSend(A_MUSIC_IS_NOT_PLAYING, i_current_music_track); // Updates the music track on the attenuator.
  }
}

void musicPrevTrack() {
  uint16_t i_temp_track = i_current_music_track; // Used for music navigation.

  // Determine the previous track.
  if(i_current_music_track - 1 < i_music_track_start) {
    // Start at the last track if already on the first.
    i_temp_track = i_music_track_start + (i_music_count - 1);
  }
  else {
    i_temp_track--;
  }

  // Switch to the previous track.
  if(b_playing_music) {
    // Stops music using the current track number as the identifier.
    stopMusic();

    i_current_music_track = i_temp_track; // Change only AFTER stopping music playback.

    // Play the appropriate track on pack and wand, and notify the Attenuator.
    playMusic();
  }
  else {
    // Set the new track.
    i_current_music_track = i_temp_track;

    attenuatorSend(A_MUSIC_IS_NOT_PLAYING, i_current_music_track); // Updates the music track on the attenuator.
  }
}

// Adjust the gain of a single track.
void adjustGainEffect(uint16_t i_track_id, int8_t i_track_volume, bool b_fade, uint16_t i_fade_time) {
  if(i_track_volume < i_volume_abs_min) {
    i_track_volume = i_volume_abs_min;
  }

  if(i_track_volume > i_volume_abs_max) {
    i_track_volume = i_volume_abs_max;
  }

  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      if(b_fade) {
        audio.trackFade(i_track_id, i_track_volume, i_fade_time, 0);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
      }
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

void updateMasterVolume(bool startup) {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.masterGain(i_volume_master);
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }

  if(!startup) {
    // If this isn't being called at boot, provide audio feedback and report the change.
    if(!b_pack_on && !b_pack_shutting_down) {
      // Provide feedback when the Proton Pack is not running.
      stopEffect(S_BEEPS_ALT);
      playEffect(S_BEEPS_ALT);
    }

    attenuatorSendData(A_VOLUME_SYNC); // Tell the connected device about this change.
  }
}

void increaseVolumeEEPROM() {
  if(i_volume_master_eeprom == i_volume_abs_max) {
    // Cannot go any higher.
  }
  else {
    if(i_volume_master_percentage + VOLUME_MULTIPLIER > 100) {
      i_volume_master_percentage = 100;
    }
    else {
      i_volume_master_percentage += VOLUME_MULTIPLIER;
    }

    i_volume_master_eeprom = (MINIMUM_VOLUME + i_volume_min_adj) - ((MINIMUM_VOLUME + i_volume_min_adj) * i_volume_master_percentage / 100);
    i_volume_master = i_volume_master_eeprom;
    i_volume_revert = i_volume_master_eeprom;

    updateMasterVolume();
  }
}

void decreaseVolumeEEPROM() {
  if(i_volume_master_eeprom == (MINIMUM_VOLUME + i_volume_min_adj)) {
    // Cannot go any lower.
  }
  else {
    if(i_volume_master_percentage - VOLUME_MULTIPLIER < 0) {
      i_volume_master_percentage = 0;
    }
    else {
      i_volume_master_percentage -= VOLUME_MULTIPLIER;
    }

    i_volume_master_eeprom = (MINIMUM_VOLUME + i_volume_min_adj) - ((MINIMUM_VOLUME + i_volume_min_adj) * i_volume_master_percentage / 100);
    i_volume_master = i_volume_master_eeprom;
    i_volume_revert = i_volume_master_eeprom;

    updateMasterVolume();
  }
}

void increaseVolume() {
  if(i_volume_master == i_volume_abs_max) {
    // Cannot go any higher.
  }
  else {
    if(i_volume_master_percentage + VOLUME_MULTIPLIER > 100) {
      i_volume_master_percentage = 100;
    }
    else {
      i_volume_master_percentage += VOLUME_MULTIPLIER;
    }

    i_volume_master = (MINIMUM_VOLUME + i_volume_min_adj) - ((MINIMUM_VOLUME + i_volume_min_adj) * i_volume_master_percentage / 100);
    i_volume_revert = i_volume_master;

    updateMasterVolume();
  }
}

void decreaseVolume() {
  if(i_volume_master == (MINIMUM_VOLUME + i_volume_min_adj)) {
    // Cannot go any lower.
  }
  else {
    if(i_volume_master_percentage - VOLUME_MULTIPLIER < 0) {
      i_volume_master_percentage = 0;
    }
    else {
      i_volume_master_percentage -= VOLUME_MULTIPLIER;
    }

    i_volume_master = (MINIMUM_VOLUME + i_volume_min_adj) - ((MINIMUM_VOLUME + i_volume_min_adj) * i_volume_master_percentage / 100);
    i_volume_revert = i_volume_master;

    updateMasterVolume();
  }
}

void updateEffectsVolume() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      // Only effects that are long or looped require adjustment.
      audio.trackGain(S_BEEP_8, i_volume_effects);
      audio.trackGain(S_WAND_BOOTUP, i_volume_effects);
      audio.trackGain(S_PACK_RIBBON_ALARM_1, i_volume_effects);
      audio.trackGain(S_ALARM_LOOP, i_volume_effects);
      audio.trackGain(S_SMASH_ERROR_LOOP, i_volume_effects);
      audio.trackGain(S_RIBBON_CABLE_START, i_volume_effects);
      audio.trackGain(S_STEAM_LOOP, i_volume_effects);
      audio.trackGain(S_SHUTDOWN, i_volume_effects);

      switch(SYSTEM_YEAR) {
        case SYSTEM_1984:
          audio.trackGain(S_GB1_1984_BOOT_UP, i_volume_effects);
          audio.trackGain(S_GB1_1984_PACK_LOOP, i_volume_effects);
        break;

        case SYSTEM_1989:
          audio.trackGain(S_GB2_PACK_START, i_volume_effects);
          audio.trackGain(S_GB2_PACK_LOOP, i_volume_effects);
        break;

        case SYSTEM_AFTERLIFE:
        case SYSTEM_FROZEN_EMPIRE:
        default:
          if(STREAM_MODE == SLIME) {
            // In slime blower mode these sounds have lower volume than normal.
            audio.trackGain(S_BOOTUP, i_volume_effects - 30);
            audio.trackGain(S_AFTERLIFE_PACK_STARTUP, i_volume_effects - 30);
            audio.trackGain(S_AFTERLIFE_PACK_IDLE_LOOP, i_volume_effects - 40);
            audio.trackGain(S_FROZEN_EMPIRE_PACK_STARTUP, i_volume_effects - 30);
            audio.trackGain(S_FROZEN_EMPIRE_PACK_IDLE_LOOP, i_volume_effects - 40);
          }
          else {
            audio.trackGain(S_BOOTUP, i_volume_effects);
            audio.trackGain(S_AFTERLIFE_PACK_STARTUP, i_volume_effects);
            audio.trackGain(S_AFTERLIFE_PACK_IDLE_LOOP, i_volume_effects);
            audio.trackGain(S_FROZEN_EMPIRE_PACK_STARTUP, i_volume_effects);
            audio.trackGain(S_FROZEN_EMPIRE_PACK_IDLE_LOOP, i_volume_effects);
          }

          audio.trackGain(S_PACK_SHUTDOWN_AFTERLIFE_ALT, i_volume_effects);
          audio.trackGain(S_FROZEN_EMPIRE_PACK_SHUTDOWN, i_volume_effects);
          audio.trackGain(S_FROZEN_EMPIRE_BRASS_SHUTDOWN, i_volume_effects);
          audio.trackGain(S_POWERCELL, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_BEEP_WAND_S1, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_BEEP_WAND_S2, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_BEEP_WAND_S3, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_BEEP_WAND_S4, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_BEEP_WAND_S5, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_WAND_RAMP_1, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_WAND_RAMP_2, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_WAND_RAMP_2_FADE_IN, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_WAND_IDLE_1, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_WAND_IDLE_2, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_WAND_RAMP_DOWN_2, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_WAND_RAMP_DOWN_2_FADE_OUT, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_AFTERLIFE_WAND_RAMP_DOWN_1, i_volume_effects - i_wand_idle_level);
          audio.trackGain(S_PACK_BEEPS_OVERHEAT, i_volume_effects);
          audio.trackGain(S_PACK_OVERHEAT_HOT, i_volume_effects);

          if(b_brass_pack_sound_loop) {
            audio.trackGain(S_FROZEN_EMPIRE_BOOT_EFFECT, i_volume_effects);
          }
        break;
      }

      switch(STREAM_MODE) {
        case PROTON:
        default:
          if(b_wand_firing) {
            audio.trackGain(S_GB1_FIRE_HIGH_POWER_LOOP, i_volume_effects);
            audio.trackGain(S_GB1_1984_FIRE_LOOP_PACK, i_volume_effects);
            audio.trackGain(S_GB1_1984_FIRE_HIGH_POWER_LOOP, i_volume_effects);
            audio.trackGain(S_GB2_FIRE_LOOP, i_volume_effects);
            audio.trackGain(S_FIRING_LOOP_GB1, i_volume_effects);
          }
        break;

        case SLIME:
          audio.trackGain(S_PACK_SLIME_TANK_LOOP, i_volume_effects);
          audio.trackGain(S_SLIME_REFILL, i_volume_effects);

          if(b_wand_firing) {
            audio.trackGain(S_SLIME_LOOP, i_volume_effects);
          }
        break;

        case STASIS:
          audio.trackGain(S_STASIS_IDLE_LOOP, i_volume_effects);

          if(b_wand_firing) {
            audio.trackGain(S_STASIS_LOOP, i_volume_effects);
          }
        break;

        case MESON:
          audio.trackGain(S_MESON_IDLE_LOOP, i_volume_effects);
        break;
      }
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }

  attenuatorSendData(A_VOLUME_SYNC); // Tell the connected device about this change.
}

void increaseVolumeEffects() {
  if(i_volume_effects_percentage + VOLUME_EFFECTS_MULTIPLIER > 100) {
    i_volume_effects_percentage = 100;

    // Provide feedback at maximum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT);
  }
  else {
    i_volume_effects_percentage += VOLUME_EFFECTS_MULTIPLIER;
  }

  i_volume_effects = i_volume_abs_min - (i_volume_abs_min * i_volume_effects_percentage / 100);

  updateEffectsVolume();
}

void decreaseVolumeEffects() {
  if(i_volume_effects_percentage - VOLUME_EFFECTS_MULTIPLIER < 0) {
    i_volume_effects_percentage = 0;

    // Provide feedback at minimum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, i_volume_master);
  }
  else {
    i_volume_effects_percentage -= VOLUME_EFFECTS_MULTIPLIER;
  }

  i_volume_effects = i_volume_abs_min - (i_volume_abs_min * i_volume_effects_percentage / 100);

  updateEffectsVolume();
}

void updateMusicVolume() {
  if(i_music_count > 0) {
    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
      case A_GPSTAR_AUDIO_ADV:
        audio.trackGain(i_current_music_track, i_volume_music);
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }
  }

  attenuatorSendData(A_VOLUME_SYNC); // Tell the connected device about this change.
}

void increaseVolumeMusic() {
  if(i_volume_music_percentage + VOLUME_MUSIC_MULTIPLIER > 100) {
    i_volume_music_percentage = 100;

    // Provide feedback at maximum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, i_volume_master);
  }
  else {
    i_volume_music_percentage += VOLUME_MUSIC_MULTIPLIER;
  }

  i_volume_music = i_volume_abs_min - (i_volume_abs_min * i_volume_music_percentage / 100);

  updateMusicVolume();
}

void decreaseVolumeMusic() {
  if(i_volume_music_percentage - VOLUME_MUSIC_MULTIPLIER < 0) {
    i_volume_music_percentage = 0;

    // Provide feedback at minimum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, i_volume_master);
  }
  else {
    i_volume_music_percentage -= VOLUME_MUSIC_MULTIPLIER;
  }

  i_volume_music = i_volume_abs_min - (i_volume_abs_min * i_volume_music_percentage / 100);

  updateMusicVolume();
}

void buildMusicCount(uint16_t i_num_tracks) {
  // Build the music track count.
  i_music_count = i_num_tracks - i_last_effects_track;

  if(i_music_count > 0 && i_music_count < 4097) {
    i_current_music_track = i_music_track_start; // Set the first track of music as file 500_
  }
  else {
    i_music_count = 0; // If the music count is corrupt, make it 0
    debugln(F("Warning: Calculated music count exceeds 4096; SD card corruption likely!"));
  }
}

bool musicIsTrackCounterReset() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      return audio.isTrackCounterReset();
    break;

    case A_NONE:
    default:
      return false;
    break;
  }
}

void musicTrackPlayingStatus() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.trackPlayingStatus(i_current_music_track);
    break;

    case A_NONE:
    default:
      // Do nothing.
    break;
  }
}

bool musicTrackStatus() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      return audio.currentTrackStatus(i_current_music_track);
    break;

    case A_NONE:
    default:
      return false;
    break;
  }
}

void checkMusic() {
  if(ms_check_music.justFinished() && !ms_music_next_track.isRunning()) {
    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
      case A_GPSTAR_AUDIO_ADV:
        ms_check_music.start(i_music_check_delay);

        musicTrackPlayingStatus();

        // Loop through all the tracks if the music is not set to repeat a track.
        if(b_playing_music && !b_repeat_track && !b_music_paused) {
          if(ms_music_status_check.justFinished()) {
            if(!musicTrackStatus() && !musicIsTrackCounterReset()) {
              ms_check_music.stop();
              ms_music_status_check.stop();

              stopMusic();

              // Switch to the next track.
              if(i_current_music_track + 1 > i_music_track_start + i_music_count - 1) {
                i_current_music_track = i_music_track_start;
              }
              else {
                i_current_music_track++;
              }

              // Start timer to prepare to play music again.
              ms_music_next_track.start(i_music_next_track_delay);
            }
            else {
              ms_music_status_check.start(i_music_check_delay * 4);
            }
          }
        }
      break;

      case A_NONE:
      default:
        // None
      break;
    }
  }

  // Start playing music again.
  if(ms_music_next_track.justFinished()) {
    ms_music_next_track.stop();
    ms_check_music.start(i_music_check_delay);

    // Play the appropriate track on the pack and wand, and notify the Attenuator.
    playMusic();
  }
}

void toggleMusicLoop() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      // Loop the music track.
      if(!b_repeat_track) {
        b_repeat_track = true;

        if(i_music_count > 0) {
          audio.trackLoop(i_current_music_track, 1);
        }
      }
      else {
        b_repeat_track = false;

        if(i_music_count > 0) {
          audio.trackLoop(i_current_music_track, 0);
        }
      }
    break;

    case A_NONE:
    default:
      if(!b_repeat_track) {
        b_repeat_track = true;
      }
      else {
        b_repeat_track = false;
      }
    break;
  }
}

/*
 * Audio Setup Routines
 * Used to detect, update, and reset the available audio devices.
 */
bool setupAudioDevice() {
  char gVersion[VERSION_STRING_LEN];

#ifdef ESP32
  AudioSerial.begin(57600, SERIAL_8N1, AUDIO_RX_PIN, AUDIO_TX_PIN);
#else
  AudioSerial.begin(57600);
#endif

  audio.start(AudioSerial);

  uint16_t i_timeout = millis() + 1000;

  while(!audio.gpstarAudioHello() && millis() < i_timeout) {
    audio.hello();
    delay(10);
  }

  if(audio.gpstarAudioHello()) {
    if(audio.getVersionNumber() != 0) {
      AUDIO_DEVICE = A_GPSTAR_AUDIO_ADV;
    }
    else {
      AUDIO_DEVICE = A_GPSTAR_AUDIO;
    }

    i_volume_min_adj = 10; // Moves minimum volume up for GPStar Audio since its minimum is higher.
    i_volume_master = (MINIMUM_VOLUME + i_volume_min_adj) - ((MINIMUM_VOLUME + i_volume_min_adj) * i_volume_master_percentage / 100); // Master overall volume.
    i_volume_master_eeprom = i_volume_master; // Master overall volume that is saved into the eeprom menu and loaded during bootup.
    i_volume_revert = i_volume_master; // Used to restore volume level from a muted state.

    debugln(F("Using GPStar Audio"));
    debug(F("Version: "));
    debugln(audio.getVersionNumber());

    buildMusicCount(audio.getNumTracks());
    audio.gpstarLEDStatus(false);

    return true;
  }

  // Reset the master gain db. Range is -70 to 0. Bootup the system muted, then we reset it after the system is loaded.
  audio.masterGain(i_volume_abs_min);

  // Stop all tracks.
  audio.stopAllTracks();

  // Ask for some WAV Trigger information.
  audio.requestVersionString();
  audio.requestSystemInfo();

  // Delay to allow time for WAV Trigger to respond.
  delay(10);

  if(audio.getVersion(gVersion)) {
    // We found a WAV Trigger. Build the music track count.
    if(audio.wasSysInfoRcvd()) {
      // Only attempt to build a music track count if the WAV Trigger responded with RSP_SYSTEM_INFO.
      buildMusicCount(audio.getNumTracks());
    }
    else {
      debugln(F("Warning: RSP_SYSTEM_INFO not received!"));
    }

    // Reset the sample rate offset. Only for the WAV Trigger.
    audio.samplerateOffset(0);

    // Onboard amplifier on or off. Only for the WAV Trigger.
    audio.setAmpPwr(b_onboard_amp_enabled);

    // Enable track reporting. Only for the WAV Trigger.
    audio.setReporting(true);

    AUDIO_DEVICE = A_WAV_TRIGGER;

    debugln(F("Using WAV Trigger"));

    return true;
  }
  else {
    // No audio devices connected.
    AUDIO_DEVICE = A_NONE;
    AudioSerial.end();

    debugln(F("No Audio Device"));

    return false;
  }
}

void updateAudio() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.update();
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }
}
