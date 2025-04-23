// Required for PlatformIO
#include <Arduino.h>

// Set to 1 to enable built-in debug messages
#define DEBUG 1

// Debug macros
#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

// 3rd-Party Libraries
#include <millisDelay.h>
#include <ezButton.h>
#include <SoftwareSerial.h>
#include <SerialTransfer.h>
#include <Wire.h>

// Local Files
#include "Configuration.h"
#include "MusicSounds.h"
#include "Communication.h"
#include "Header.h"
#include "Colours.h"
#include "Audio.h"
#include "Preferences.h"
#include "System.h"
#include "Serial.h"

void setup() {
  // Initialize serial communication at 9600 baud
  Serial.begin(9600);

  // Initialize I2C as master
  Wire.begin();

  // Set the LED pin as an output
  pinMode(LED_PIN, OUTPUT);

  // Turn the LED off initially (active low)
  digitalWrite(LED_PIN, HIGH);

  // Setup the audio device for this controller.
  setupAudioDevice();

  if (AUDIO_DEVICE == A_GPSTAR_AUDIO || AUDIO_DEVICE == A_GPSTAR_AUDIO_ADV) {
    updateAudio(); // Update the state of the available sound board.
    updateMasterVolume(true); // Reset the master volume manually.
  }
}

void loop() {
  delay(4000); // Wait for 4 seconds.

  if (AUDIO_DEVICE == A_GPSTAR_AUDIO || AUDIO_DEVICE == A_GPSTAR_AUDIO_ADV) {
    digitalWrite(LED_PIN, LOW); // Turn on the LED to indicate we found an audio device.

    updateAudio(); // Update the state of the available sound board.

    debugln(F("Playing audio effect..."));

    playEffect(S_BOOTUP);
  }
}
