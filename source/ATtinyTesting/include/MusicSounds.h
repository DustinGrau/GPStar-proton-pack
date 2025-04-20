#pragma once

/*
 * Micro SD Card sound files in order. If you have no sound, your SD card might be too slow, try a faster one.
 * File naming 000_ is important. For music, it is 500_ and higher.
 */

enum sound_fx {
  S_EMPTY,
  S_BOOTUP,
  S_SHUTDOWN
};

/*
 * Need to keep track which is the last sound effect, so we can iterate over the effects to adjust the volume gain on them.
 */
const uint16_t i_last_effects_track = S_SHUTDOWN;