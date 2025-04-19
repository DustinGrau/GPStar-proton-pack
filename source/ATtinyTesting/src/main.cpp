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
#include <FastLED.h>
#include <ezButton.h>
#include <SoftwareSerial.h>
#include <SerialTransfer.h>
#include <Wire.h>

// Local Files
#include "Configuration.h"
#include "Header.h"
#include "Audio.h"

void setup() {
  // Initialize serial communication at 9600 baud
  Serial.begin(9600);

  // Optional: Print a message to confirm initialization
  debugln("Serial communication initialized.");

  // Initialize I2C as master
  Wire.begin();

  // Optional: Print a message to confirm initialization
  debugln("I2C initialized.");

  // Set the LED pin as an output
  pinMode(LED_PIN, OUTPUT);

  // Turn the LED off initially (active low)
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  // Turn the LED on (active low)
  digitalWrite(LED_PIN, LOW);
  delay(500); // Wait for 500 milliseconds

  // Turn the LED off
  digitalWrite(LED_PIN, HIGH);
  delay(500); // Wait for 500 milliseconds

  // Send a message every loop
  debugln("Hello, ATtiny816!");
}
